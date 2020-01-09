/*
 * $Id: b_get.c,v 1.14 2008/01/31 17:01:22 bostic Exp $
 */
#include "bench.h"

static int b_get_usage(void);

int
b_get(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB *dbp;
	DBTYPE type;
	DBT key, data;
	db_recno_t recno;
	u_int32_t cachesize;
	int ch, i, count;
	char *ts;

	type = DB_BTREE;
	cachesize = MEGABYTE;
	count = 100000;
	ts = "Btree";
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "C:c:t:")) != EOF)
		switch (ch) {
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 't':
			switch (optarg[0]) {
			case 'B': case 'b':
				ts = "Btree";
				type = DB_BTREE;
				break;
			case 'H': case 'h':
				if (b_util_have_hash())
					return (0);
				ts = "Hash";
				type = DB_HASH;
				break;
			case 'Q': case 'q':
				if (b_util_have_queue())
					return (0);
				ts = "Queue";
				type = DB_QUEUE;
				break;
			case 'R': case 'r':
				ts = "Recno";
				type = DB_RECNO;
				break;
			default:
				return (b_get_usage());
			}
			break;
		case '?':
		default:
			return (b_get_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (b_get_usage());

	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	DB_BENCH_ASSERT(dbp->set_cachesize(dbp, 0, cachesize, 0) == 0);
	dbp->set_errfile(dbp, stderr);

	/* Set record length for Queue. */
	if (type == DB_QUEUE)
		DB_BENCH_ASSERT(dbp->set_re_len(dbp, 10) == 0);

#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(
	    dbp->open(dbp, NULL, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(
	    dbp->open(dbp, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#endif

	/* Store a key/data pair. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	switch (type) {
	case DB_BTREE:
	case DB_HASH:
		key.data = "aaaaa";
		key.size = 5;
		break;
	case DB_QUEUE:
	case DB_RECNO:
		recno = 1;
		key.data = &recno;
		key.size = sizeof(recno);
		break;
	case DB_UNKNOWN:
		b_util_abort();
		break;
	}
	data.data = "bbbbb";
	data.size = 5;

	DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);

	/* Retrieve the key/data pair count times. */
	TIMER_START;
	for (i = 0; i < count; ++i)
		DB_BENCH_ASSERT(dbp->get(dbp, NULL, &key, &data, 0) == 0);
	TIMER_STOP;

	printf("# %d %s database get of cached key/data item\n", count, ts);
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	return (0);
}

static int
b_get_usage()
{
	(void)fprintf(stderr,
	    "usage: b_get [-C cachesz] [-c count] [-t type]\n");
	return (EXIT_FAILURE);
}
