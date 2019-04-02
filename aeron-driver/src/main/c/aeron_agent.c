/*
 * Copyright 2014-2019 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__linux__)
#define _BSD_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "concurrent/aeron_thread.h"
#include "aeron_agent.h"
#include "aeron_alloc.h"
#include "aeron_driver_context.h"
#include "util/aeron_error.h"
#include "util/aeron_dlopen.h"
#include "util/aeron_parse_util.h"

void aeron_idle_strategy_sleeping_idle(void *state, int work_count)
{
    uint64_t *duration = (uint64_t *)state;

    if (work_count > 0)
    {
        return;
    }

    aeron_nano_sleep(*duration);
}

int aeron_idle_strategy_sleeping_init_args(void **state, const char *env_var, const char *init_args)
{
    if (aeron_alloc((void **)state, sizeof(uint64_t)) < 0)
    {
        int err_code = errno;

        aeron_set_err(err_code, "%s:%d: %s", __FILE__, __LINE__, strerror(err_code));
        return -1;
    }

    uint64_t *duration = (uint64_t *)*state;
    if (NULL == init_args)
    {
        *duration = 1;
    }
    else
    {
        return aeron_parse_duration_ns(init_args, duration);
    }

    return 0;
}

void aeron_idle_strategy_yielding_idle(void *state, int work_count)
{
    if (work_count > 0)
    {
        return;
    }

    sched_yield();
}

void aeron_idle_strategy_busy_spinning_idle(void *state, int work_count)
{
    if (work_count > 0)
    {
        return;
    }

    proc_yield();
}

void aeron_idle_strategy_noop_idle(void *state, int work_count)
{
}

#define AERON_IDLE_STRATEGY_BACKOFF_STATE_NOT_IDLE 0
#define AERON_IDLE_STRATEGY_BACKOFF_STATE_SPINNING 1
#define AERON_IDLE_STRATEGY_BACKOFF_STATE_YIELDING 2
#define AERON_IDLE_STRATEGY_BACKOFF_STATE_PARKING 3

typedef struct aeron_idle_strategy_backoff_state_stct
{
    uint8_t pre_pad[AERON_CACHE_LINE_LENGTH * 2];
    uint64_t max_spins;
    uint64_t max_yields;
    uint64_t min_park_period_ns;
    uint64_t max_park_period_ns;
    uint64_t spins;
    uint64_t yields;
    uint64_t park_period_ns;
    uint8_t state;
    uint8_t post_pad[AERON_CACHE_LINE_LENGTH * 2];
}
aeron_idle_strategy_backoff_state_t;

void aeron_idle_strategy_backoff_idle(void *state, int work_count)
{
    aeron_idle_strategy_backoff_state_t *backoff_state = (aeron_idle_strategy_backoff_state_t *)state;

    if (work_count > 0)
    {
        backoff_state->spins = 0;
        backoff_state->yields = 0;
        backoff_state->park_period_ns = backoff_state->min_park_period_ns;
        backoff_state->state = AERON_IDLE_STRATEGY_BACKOFF_STATE_NOT_IDLE;
    }
    else
    {
        switch (backoff_state->state)
        {
            case AERON_IDLE_STRATEGY_BACKOFF_STATE_NOT_IDLE:
                backoff_state->state = AERON_IDLE_STRATEGY_BACKOFF_STATE_SPINNING;
                backoff_state->spins++;

                break;

            case AERON_IDLE_STRATEGY_BACKOFF_STATE_SPINNING:
                proc_yield();
                if (++backoff_state->spins > backoff_state->max_spins)
                {
                    backoff_state->state = AERON_IDLE_STRATEGY_BACKOFF_STATE_YIELDING;
                    backoff_state->yields = 0;
                }
                break;

            case AERON_IDLE_STRATEGY_BACKOFF_STATE_YIELDING:
                if (++backoff_state->yields > backoff_state->max_yields)
                {
                    backoff_state->state = AERON_IDLE_STRATEGY_BACKOFF_STATE_PARKING;
                    backoff_state->park_period_ns = backoff_state->min_park_period_ns;
                }
                else
                {
                    sched_yield();
                }
                break;

            case AERON_IDLE_STRATEGY_BACKOFF_STATE_PARKING:
            default:
                aeron_nano_sleep(backoff_state->park_period_ns);
                backoff_state->park_period_ns =
                    ((backoff_state->park_period_ns << 1) < backoff_state->max_park_period_ns) ?
                    backoff_state->park_period_ns << 1 : backoff_state->max_park_period_ns;
                break;
        }
    }
}

int aeron_idle_strategy_backoff_state_init(
    void **state, uint64_t max_spins, uint64_t max_yields, uint64_t min_park_period_ns, uint64_t max_park_period_ns)
{
    if (aeron_alloc((void **)state, sizeof(aeron_idle_strategy_backoff_state_t)) < 0)
    {
        int err_code = errno;

        aeron_set_err(err_code, "%s:%d: %s", __FILE__, __LINE__, strerror(err_code));
        return -1;
    }

    aeron_idle_strategy_backoff_state_t *backoff_state = (aeron_idle_strategy_backoff_state_t *)*state;

    backoff_state->max_spins = max_spins;
    backoff_state->max_yields = max_yields;
    backoff_state->min_park_period_ns = min_park_period_ns;
    backoff_state->max_park_period_ns = max_park_period_ns;
    backoff_state->spins = 0;
    backoff_state->yields = 0;
    backoff_state->park_period_ns = backoff_state->min_park_period_ns;

    return 0;
}

static int aeron_idle_strategy_backoff_state_init_args(void **state, const char *env_var, const char *init_args)
{
    if (NULL == init_args)
    {
        return aeron_idle_strategy_backoff_state_init(
            state,
            AERON_IDLE_STRATEGY_BACKOFF_MAX_SPINS,
            AERON_IDLE_STRATEGY_BACKOFF_MAX_YIELDS,
            AERON_IDLE_STRATEGY_BACKOFF_MIN_PARK_PERIOD_NS,
            AERON_IDLE_STRATEGY_BACKOFF_MAX_PARK_PERIOD_NS);
    }

    char spins_str[17], yields_str[17], min_park_str[17], max_park_str[17];

    int matches = sscanf(
        init_args,
        "%16[,.0-9]-%16[,.0-9]-%16[,.0-9mus]-%16[,.0-9mus]",
        spins_str,
        yields_str,
        min_park_str,
        max_park_str);

    if (4 != matches)
    {
        aeron_set_err(EINVAL, "%s:%d: %s", __FILE__, __LINE__, "init args malformed");
        return -1;
    }

    errno = 0;
    char *end_ptr = NULL;
    uint64_t max_spins = strtoull(spins_str, &end_ptr, 10);
    if ((0 == max_spins && 0 != errno) || end_ptr == spins_str)
    {
        int err_code = errno;

        aeron_set_err(err_code, "%s:%d: %s", __FILE__, __LINE__, "max spins not parseable");
        return -1;
    }

    errno = 0;
    end_ptr = NULL;
    uint64_t max_yields = strtoull(yields_str, &end_ptr, 10);
    if ((0 == max_yields && 0 != errno) || end_ptr == yields_str)
    {
        int err_code = errno;

        aeron_set_err(err_code, "%s:%d: %s", __FILE__, __LINE__, "max yields not parseable");
        return -1;
    }

    uint64_t min_park_ns;
    if (aeron_parse_duration_ns(min_park_str, &min_park_ns) < 0)
    {
        aeron_set_err(EINVAL, "%s:%d: %s", __FILE__, __LINE__, "min park period ns not parseable");
        return -1;
    }

    uint64_t max_park_ns;
    if (aeron_parse_duration_ns(max_park_str, &max_park_ns) < 0)
    {
        aeron_set_err(EINVAL, "%s:%d: %s", __FILE__, __LINE__, "max park period ns not parseable");
        return -1;
    }

    return aeron_idle_strategy_backoff_state_init(state, max_spins, max_yields, min_park_ns, max_park_ns);
}

int aeron_idle_strategy_init_null(void **state, const char *env_var, const char *init_args)
{
    *state = NULL;
    return 0;
}

aeron_idle_strategy_t aeron_idle_strategy_sleeping =
    {
        aeron_idle_strategy_sleeping_idle,
        aeron_idle_strategy_sleeping_init_args
    };

aeron_idle_strategy_t aeron_idle_strategy_yielding =
    {
        aeron_idle_strategy_yielding_idle,
        aeron_idle_strategy_init_null
    };

aeron_idle_strategy_t aeron_idle_strategy_busy_spinning =
    {
        aeron_idle_strategy_busy_spinning_idle,
        aeron_idle_strategy_init_null
    };

aeron_idle_strategy_t aeron_idle_strategy_noop =
    {
        aeron_idle_strategy_noop_idle,
        aeron_idle_strategy_init_null
    };

aeron_idle_strategy_t aeron_idle_strategy_backoff =
    {
        aeron_idle_strategy_backoff_idle,
        aeron_idle_strategy_backoff_state_init_args
    };

aeron_idle_strategy_func_t aeron_idle_strategy_load(
    const char *idle_strategy_name,
    void **idle_strategy_state,
    const char *env_var,
    const char *init_args)
{
    char idle_func_name[AERON_MAX_PATH];
    aeron_idle_strategy_func_t idle_func = NULL;
    aeron_idle_strategy_init_func_t idle_init_func = NULL;
    void *idle_state = NULL;

    if (NULL == idle_strategy_name || NULL == idle_strategy_state)
    {
        aeron_set_err(EINVAL, "%s", "invalid idle strategy name or state");
        return NULL;
    }

    *idle_strategy_state = NULL;

    if (strncmp(idle_strategy_name, "sleeping", sizeof("sleeping")) == 0)
    {
        return aeron_idle_strategy_load("aeron_idle_strategy_sleeping", idle_strategy_state, env_var, init_args);
    }
    else if (strncmp(idle_strategy_name, "yielding", sizeof("yielding")) == 0)
    {
        return aeron_idle_strategy_load("aeron_idle_strategy_yielding", idle_strategy_state, env_var, init_args);
    }
    else if (strncmp(idle_strategy_name, "spinning", sizeof("spinning")) == 0)
    {
        return aeron_idle_strategy_load("aeron_idle_strategy_busy_spinning", idle_strategy_state, env_var, init_args);
    }
    else if (strncmp(idle_strategy_name, "noop", sizeof("noop")) == 0)
    {
        return aeron_idle_strategy_load("aeron_idle_strategy_noop", idle_strategy_state, env_var, init_args);
    }
    else if (strncmp(idle_strategy_name, "backoff", sizeof("backoff")) == 0)
    {
        return aeron_idle_strategy_load("aeron_idle_strategy_backoff", idle_strategy_state, env_var, init_args);
    }
    else
    {
        aeron_idle_strategy_t *idle_strat = NULL;

        snprintf(idle_func_name, sizeof(idle_func_name) - 1, "%s", idle_strategy_name);
        if ((idle_strat = (aeron_idle_strategy_t *)aeron_dlsym(RTLD_DEFAULT, idle_func_name)) == NULL)
        {
            aeron_set_err(EINVAL, "could not find idle strategy %s: dlsym - %s", idle_func_name, aeron_dlerror());
            return NULL;
        }

        idle_func = idle_strat->idle;
        idle_init_func = idle_strat->init;

        if (idle_init_func(&idle_state, env_var, init_args) < 0)
        {
            return NULL;
        }

        *idle_strategy_state = idle_state;
    }

    return idle_func;
}

aeron_agent_on_start_func_t aeron_agent_on_start_load(const char *name)
{
    aeron_agent_on_start_func_t func = NULL;
    if ((func = (aeron_agent_on_start_func_t)aeron_dlsym(RTLD_DEFAULT, name)) == NULL)
    {
        aeron_set_err(EINVAL, "could not find agent on_start func %s: dlsym - %s", name, aeron_dlerror());
        return NULL;
    }

    return func;
}

int aeron_agent_init(
    aeron_agent_runner_t *runner,
    const char *role_name,
    void *state,
    aeron_agent_on_start_func_t on_start,
    void *on_start_state,
    aeron_agent_do_work_func_t do_work,
    aeron_agent_on_close_func_t on_close,
    aeron_idle_strategy_func_t idle_strategy_func,
    void *idle_strategy_state)
{
    size_t role_name_length = strlen(role_name);

    if (NULL == runner || NULL == do_work || NULL == idle_strategy_func)
    {
        aeron_set_err(EINVAL, "%s", "invalid argument");
        return -1;
    }

    runner->agent_state = state;
    runner->on_start = on_start;
    runner->on_start_state = on_start_state;
    runner->do_work = do_work;
    runner->on_close = on_close;
    if (aeron_alloc((void **)&runner->role_name, role_name_length + 1) < 0)
    {
        int err_code = errno;

        aeron_set_err(err_code, "%s:%d: %s", __FILE__, __LINE__, strerror(err_code));
        return -1;
    }
    memcpy((char *)runner->role_name, role_name, role_name_length);

    runner->idle_strategy_state = idle_strategy_state;
    runner->idle_strategy = idle_strategy_func;
    runner->running = true;
    runner->state = AERON_AGENT_STATE_INITED;

    return 0;
}

static void *agent_main(void *arg)
{
    aeron_agent_runner_t *runner = (aeron_agent_runner_t *)arg;

#if defined(Darwin)
    aeron_thread_set_name(runner->role_name);
#else
    aeron_thread_set_name(aeron_thread_self(), runner->role_name);
#endif

    if (NULL != runner->on_start)
    {
        runner->on_start(runner->on_start_state, runner->role_name);
    }

    while (aeron_agent_is_running(runner))
    {
        runner->idle_strategy(runner->idle_strategy_state, runner->do_work(runner->agent_state));
    }

    return NULL;
}

int aeron_agent_start(aeron_agent_runner_t *runner)
{
    pthread_attr_t attr;
    int pthread_result = 0;

    if (NULL == runner)
    {
        aeron_set_err(EINVAL, "%s", "invalid argument");
        return -1;
    }

    if ((pthread_result = aeron_thread_attr_init(&attr)) != 0)
    {
        aeron_set_err(pthread_result, "aeron_thread_attr_init: %s", strerror(pthread_result));
        return -1;
    }

    if ((pthread_result = aeron_thread_create(&runner->thread, &attr, agent_main, runner)) != 0)
    {
        aeron_set_err(pthread_result, "aeron_thread_create: %s", strerror(pthread_result));
        return -1;
    }

    runner->state = AERON_AGENT_STATE_STARTED;

    return 0;
}

extern int aeron_agent_do_work(aeron_agent_runner_t *runner);
extern bool aeron_agent_is_running(aeron_agent_runner_t *runner);
extern void aeron_agent_idle(aeron_agent_runner_t *runner, int work_count);

int aeron_agent_stop(aeron_agent_runner_t *runner)
{
    int pthread_result = 0;

    if (NULL == runner)
    {
        aeron_set_err(EINVAL, "%s", "invalid argument");
        return -1;
    }

    AERON_PUT_ORDERED(runner->running, false);

    if (AERON_AGENT_STATE_STARTED == runner->state)
    {
        runner->state = AERON_AGENT_STATE_STOPPING;

        if ((pthread_result = aeron_thread_join(runner->thread, NULL)))
        {
            aeron_set_err(pthread_result, "aeron_thread_join: %s", strerror(pthread_result));
            return -1;
        }

        runner->state = AERON_AGENT_STATE_STOPPED;
    }
    else if (AERON_AGENT_STATE_MANUAL == runner->state)
    {
        runner->state = AERON_AGENT_STATE_STOPPED;
    }

    return 0;
}

int aeron_agent_close(aeron_agent_runner_t *runner)
{
    if (NULL == runner)
    {
        aeron_set_err(EINVAL, "%s", "invalid argument");
        return -1;
    }

    aeron_free((char *)runner->role_name);

    if (NULL != runner->on_close)
    {
        runner->on_close(runner->agent_state);
    }

    return 0;
}

