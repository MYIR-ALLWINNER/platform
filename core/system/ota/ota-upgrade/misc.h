#ifndef __MISC_H__
#define __MISC_H__

int get_env(void);
int update_part(const char *, const char *, loff_t);
int parse_init(void);
int firmware_update();

#endif  //__MISC_H__