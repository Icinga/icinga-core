
/*
filesystems to use for distributing index and data. In low frequency environments
this can be the same. trailing slash is mandantory
*/
DEFINE DATAFS=./
DEFINE IDXFS=./
DEFINE LOBFS=./
/*
icinga tablespaces and user must fit definitions in create_icinga_objects_oracle.sql
*/

DEFINE DATATBS=ICINGA_DATA1
DEFINE IDXTBS=ICINGA_IDX1
DEFINE LOBTBS=ICINGA_LOB1
DEFINE ICINGA_USER=icinga
DEFINE ICINGA_PASSWORD=icinga

