#ifndef __STATS_PROFILER_INCLUDED__
#define __STATS_PROFILER_INCLUDED__

/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD

typedef struct profile_object{

    char* name;
    int count;
    double elapsed;
    struct profile_object* next;

}profile_object;

profile_object* profiled_data_find_last_object();
profile_object* profile_object_create(char * name);
profile_object* profile_object_find_by_name(char * name);
double safe_divide(double x, int y, int reverse);
void profile_object_update_count(char * name, int val);
void profile_object_update_elapsed(char * name, double val);
void profile_data_print();
void profile_data_output_mrtg(char * name,char * delim);

#endif

#endif
