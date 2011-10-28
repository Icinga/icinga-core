/************************************************************************
 *
 * SLA.H - SLA functions
 * Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/


#ifndef _SLA_H
#define	_SLA_H

/*
 * A single entry in the SLA state history.
 */
typedef struct sla_state_s {
	int persistent;
	unsigned long slahistory_id;
	unsigned long instance_id;
	time_t start_time;
	time_t end_time;
	time_t acknowledgement_time;
	unsigned long object_id;
	int state;
	int state_type;
	int scheduled_downtime;
} sla_state_t;

/*
 * A list of multiple SLA state history entries.
 */

typedef struct sla_state_list_s {
	unsigned int count;
	sla_state_t states[0];
} sla_state_list_t;

/**
 * A downtime entry.
 */
typedef struct sla_downtime_s {
	unsigned long downtimehistory_id;
	unsigned long instance_id;
	unsigned long object_id;
	int is_fixed;
	int duration;
	time_t scheduled_start_time;
	time_t scheduled_end_time;
	time_t actual_start_time;
	time_t actual_end_time;
} sla_downtime_t;

/**
 * A list of multiple downtime entries.
 */
typedef struct sla_downtime_list_s {
	unsigned int count;
	sla_downtime_t downtimes[0];
} sla_downtime_list_t;

sla_state_t *sla_alloc_state(unsigned long instance_id,
                             unsigned long object_id);
void sla_free_state(sla_state_t *state);

sla_state_list_t *sla_realloc_state_list(sla_state_list_t *ptr,
        unsigned int count);
sla_state_list_t *sla_alloc_state_list(unsigned int count);
void sla_free_state_list(sla_state_list_t *list);

int sla_query_states(ido2db_idi *idi, unsigned long object_id,
                     time_t start_time, time_t end_time, sla_state_list_t **states);
int sla_save_state(ido2db_idi *idi, sla_state_t *state);
int sla_delete_state(ido2db_idi *idi, sla_state_t *state);

sla_downtime_t *sla_alloc_downtime(unsigned long instance_id,
                                   unsigned long object_id);
void sla_free_downtime(sla_downtime_t *state);

sla_downtime_list_t *sla_alloc_downtime_list(unsigned int count);
void sla_free_downtime_list(sla_downtime_list_t *list);

int sla_query_downtime(ido2db_idi *idi, unsigned long object_id,
                       time_t start_time, time_t end_time, sla_downtime_list_t **list);
int sla_apply_downtime(ido2db_idi *idi, sla_state_list_t **state_list_ptr,
                       sla_downtime_list_t *downtime_list);

int sla_process_statechange(ido2db_idi *idi, unsigned long object_id,
                            time_t start_time, time_t end_time, const int *pstate_value,
                            const int *pstate_type, const int *pdowntime);
int sla_process_acknowledgement(ido2db_idi *idi, unsigned long object_id,
                                time_t state_time, int is_acknowledged);
int sla_process_downtime(ido2db_idi *idi, unsigned long object_id,
                         time_t state_time, int event_type);
int sla_process_downtime_history(ido2db_idi *idi, unsigned long object_id,
                                 time_t start_time, time_t end_time);

#endif	/* _SLA_H */
