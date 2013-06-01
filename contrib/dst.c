#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define DAY (24*60*60)
#define YEAR (365*DAY)
#define WIDTH 5

void get_time_diff(time_t t,int *timediff,int *is_dst) {
  struct tm tm_utc,tm_local;
  tm_utc = *(gmtime(&t));
  tm_local = *(localtime(&t));

  *timediff = mktime(&tm_utc) - mktime(&tm_local);
  *is_dst = tm_local.tm_isdst;
}

int main(void) {
  time_t tnow = time(NULL);
  int is_dst,timediff;
  int last_dst,lastdiff;
  time_t endt = tnow + WIDTH*YEAR;
  time_t t = tnow - WIDTH*YEAR;
  time_t high,low;
  int count=0;

  while (1) {

    get_time_diff(t,&lastdiff,&last_dst);
    low = t;

    /* run forward till we get a change */
    for (timediff = lastdiff, is_dst = last_dst;
	 timediff == lastdiff && last_dst == is_dst && t < endt ; 
	 t += 10*DAY)
      get_time_diff(t,&timediff,&is_dst);

    if (t >= endt) break;

    high = t;

    /* find the change */
    while (low+1 < high) {
      t = (low+high)/2;
      get_time_diff(t,&timediff,&is_dst);
      if (timediff == lastdiff && is_dst == last_dst) 
	low = t;
      else
	high = t;
    }
    t = high;

    get_time_diff(t,&timediff,&is_dst);
    printf("TimeDiff=%d is_dst=%d at %s",
	   timediff,is_dst,asctime(localtime(&t)));
    count++;
  }

  if (!count)
    printf("No timezone or DST changes?\n");

  return(0);
}
