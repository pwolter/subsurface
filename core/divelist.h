// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELIST_H
#define DIVELIST_H

#include "dive.h"

#ifdef __cplusplus
extern "C" {
#endif

struct deco_state;

/* this is used for both git and xml format */
#define DATAFORMAT_VERSION 3

extern void update_cylinder_related_info(struct dive *);
extern void mark_divelist_changed(bool);
extern int unsaved_changes(void);
extern int init_decompression(struct deco_state *ds, struct dive *dive);

/* divelist core logic functions */
extern void process_loaded_dives();
/* flags for process_imported_dives() */
#define IMPORT_PREFER_IMPORTED (1 << 0)
#define	IMPORT_IS_DOWNLOADED (1 << 1)
#define	IMPORT_MERGE_ALL_TRIPS (1 << 2)
#define	IMPORT_ADD_TO_NEW_TRIP (1 << 3)
extern void add_imported_dives(struct dive_table *import_table, struct trip_table *import_trip_table, struct dive_site_table *import_sites_table,
			       int flags);
extern void process_imported_dives(struct dive_table *import_table, struct trip_table *import_trip_table, struct dive_site_table *import_sites_table,
				   int flags,
				   struct dive_table *dives_to_add, struct dive_table *dives_to_remove,
				   struct trip_table *trips_to_add, struct dive_site_table *sites_to_add);
extern char *get_dive_gas_string(const struct dive *dive);

extern int dive_table_get_insertion_index(struct dive_table *table, struct dive *dive);
extern void add_to_dive_table(struct dive_table *table, int idx, struct dive *dive);
extern void append_dive(struct dive *dive);
extern void insert_dive(struct dive_table *table, struct dive *d);
extern void get_dive_gas(const struct dive *dive, int *o2_p, int *he_p, int *o2low_p);
extern int get_divenr(const struct dive *dive);
extern int remove_dive(const struct dive *dive, struct dive_table *table);
extern bool consecutive_selected();
extern void select_dive(struct dive *dive);
extern void deselect_dive(struct dive *dive);
extern void filter_dive(struct dive *d, bool shown);
extern struct dive *first_selected_dive();
extern struct dive *last_selected_dive();
extern int get_dive_nr_at_idx(int idx);
extern void set_dive_nr_for_current_dive();
extern timestamp_t get_surface_interval(timestamp_t when);
extern void delete_dive_from_table(struct dive_table *table, int idx);
extern struct dive *find_next_visible_dive(timestamp_t when);

extern int comp_dives(const struct dive *a, const struct dive *b);

int get_min_datafile_version();
void reset_min_datafile_version();
void report_datafile_version(int version);
int get_dive_id_closest_to(timestamp_t when);
void clear_dive_file_data();
void clear_dive_table(struct dive_table *table);
void move_dive_table(struct dive_table *src, struct dive_table *dst);

#ifdef DEBUG_TRIP
extern void dump_selection(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // DIVELIST_H
