#ifndef __MAIN_H_
#define __MAIN_H_

void gameloop(void);
static inline void run_tick();
void update_fps();
void InitNewGame(bool with_intro);
void AppMinimized(void);
static void fatal(const char *str);
static bool check_data_exists();
void visible_warning(const char *fmt, ...);
void speed_test(void);
void speed_test(void);

#endif