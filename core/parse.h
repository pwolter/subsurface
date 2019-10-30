// SPDX-License-Identifier: GPL-2.0
#ifndef PARSE_H
#define PARSE_H

#define MAX_EVENT_NAME 128

#include <sqlite3.h>

typedef union {
	struct event event;
	char allocation[sizeof(struct event) + MAX_EVENT_NAME];
} event_allocation_t;

/*
 * Dive info as it is being built up..
 */

struct parser_settings {
	struct {
		const char *model;
		uint32_t deviceid;
		const char *nickname, *serial_nr, *firmware;
	} dc;
};

enum import_source {
	UNKNOWN,
	LIBDIVECOMPUTER,
	DIVINGLOG,
	UDDF,
};

/*
 * parser_state is the state needed by the parser(s). It is initialized
 * with init_parser_state() and resources are freed with free_parser_state().
 * "owning" marks pointers to objects that are freed in free_parser_state().
 * In contrast, "non-owning" marks pointers to objects that are owned
 * by other data-structures.
 */
struct parser_state {
	bool metric;
	struct parser_settings cur_settings;
	enum import_source import_source;

	struct divecomputer *cur_dc;		/* non-owning */
	struct dive *cur_dive;			/* owning */
	struct dive_site *cur_dive_site;	/* owning */
	location_t cur_location;
	struct dive_trip *cur_trip;		/* owning */
	struct sample *cur_sample;		/* non-owning */
	struct picture *cur_picture;		/* owning */
	char *country, *city;			/* owning */

	bool in_settings;
	bool in_userid;
	struct tm cur_tm;
	int cur_cylinder_index;
	int lastcylinderindex, next_o2_sensor;
	int o2pressure_sensor;
	int sample_rate;
	struct extra_data cur_extra_data;
	struct units xml_parsing_units;
	struct dive_table *target_table;	/* non-owning */
	struct trip_table *trips;		/* non-owning */
	struct dive_site_table *sites;		/* non-owning */

	sqlite3 *sql_handle;			/* for SQL based parsers */
	event_allocation_t event_allocation;
};

#define cur_event event_allocation.event

#ifdef __cplusplus
extern "C" {
#endif
void init_parser_state(struct parser_state *state);
void free_parser_state(struct parser_state *state);

/* the dive table holds the overall dive list; target table points at
 * the table we are currently filling */
extern struct dive_table dive_table;

int trimspace(char *buffer);
void start_match(const char *type, const char *name, char *buffer);
void nonmatch(const char *type, const char *name, char *buffer);
void event_start(struct parser_state *state);
void event_end(struct parser_state *state);
struct divecomputer *get_dc(struct parser_state *state);

bool is_dive(struct parser_state *state);
void reset_dc_info(struct divecomputer *dc, struct parser_state *state);
void reset_dc_settings(struct parser_state *state);
void settings_start(struct parser_state *state);
void settings_end(struct parser_state *state);
void dc_settings_start(struct parser_state *state);
void dc_settings_end(struct parser_state *state);
void dive_site_start(struct parser_state *state);
void dive_site_end(struct parser_state *state);
void dive_start(struct parser_state *state);
void dive_end(struct parser_state *state);
void trip_start(struct parser_state *state);
void trip_end(struct parser_state *state);
void picture_start(struct parser_state *state);
void picture_end(struct parser_state *state);
void cylinder_start(struct parser_state *state);
void cylinder_end(struct parser_state *state);
void ws_start(struct parser_state *state);
void ws_end(struct parser_state *state);

void sample_start(struct parser_state *state);
void sample_end(struct parser_state *state);
void divecomputer_start(struct parser_state *state);
void divecomputer_end(struct parser_state *state);
void userid_start(struct parser_state *state);
void userid_stop(struct parser_state *state);
void utf8_string(char *buffer, void *_res);

void add_dive_site(char *ds_name, struct dive *dive, struct parser_state *state);
int atoi_n(char *ptr, unsigned int len);

int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_shearwater_cloud_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_dlf_buffer(unsigned char *buffer, size_t size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
#ifdef __cplusplus
}
#endif

#endif
