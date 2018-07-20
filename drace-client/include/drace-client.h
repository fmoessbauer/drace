#pragma once

// Events
static void event_exit(void);
static void event_thread_init(void *drcontext);
static void event_thread_exit(void *drcontext);

// Runtime Configuration
static void parse_args(int argc, const char **argv);
static void print_config();

static void generate_summary();