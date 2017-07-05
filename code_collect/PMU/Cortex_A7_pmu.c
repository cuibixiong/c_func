#include "v7_pmu.h"
#include <stdio.h>

typedef struct event {
    unsigned int counter0_event;
    unsigned int counter1_event;
    unsigned int counter2_event;
    unsigned int counter3_event;
    unsigned int counter4_event;
    unsigned int counter5_event;
} PMU_EVENT_SETTING;

typedef struct value {
    unsigned int cycle;
    unsigned int overflow;
    unsigned int counter0_value;
    unsigned int counter1_value;
    unsigned int counter2_value;
    unsigned int counter3_value;
    unsigned int counter4_value;
    unsigned int counter5_value;
} PMU_EVENT_VALUE;

void pmu_start(PMU_EVENT_SETTING *);
void pmu_stop(PMU_EVENT_VALUE *);

void pmu_start(PMU_EVENT_SETTING *setting)
{
    enable_pmu();
    reset_ccnt();
    reset_pmn();
    pmn_config(0, setting->counter0_event);
    pmn_config(1, setting->counter1_event);
    pmn_config(2, setting->counter2_event);
    pmn_config(3, setting->counter3_event);
    pmn_config(4, setting->counter4_event);
    pmn_config(5, setting->counter5_event);

    enable_ccnt();
    enable_pmn(0);
    enable_pmn(1);
    enable_pmn(2);
    enable_pmn(3);
    enable_pmn(4);
    enable_pmn(5);
}

void pmu_stop(PMU_EVENT_VALUE *value)
{
    disable_ccnt();
    disable_pmn(0);
    disable_pmn(1);
    disable_pmn(2);
    disable_pmn(3);
    disable_pmn(4);
    disable_pmn(5);

    value->counter0_value = read_pmn(0);
    value->counter1_value = read_pmn(1);
    value->counter2_value = read_pmn(2);
    value->counter3_value = read_pmn(3);
    value->counter4_value = read_pmn(4);
    value->counter5_value = read_pmn(5);

    value->cycle_count = read_ccnt();
    value->overflow=read_flags();
}
