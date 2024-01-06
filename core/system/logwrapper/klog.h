#ifndef __KLOG_H__
#define __KLOG_H__

#include <syslog.h>

#define OPTION_KLOG_LEVEL_CLOSE      0
#define OPTION_KLOG_LEVEL_DEBUG      1
#define OPTION_KLOG_LEVEL_WARNING    2
#define OPTION_KLOG_LEVEL_DEFAULT    3
#define OPTION_KLOG_LEVEL_DETAIL     4

#define KLOG_LEVEL_ERROR     "E/"
#define KLOG_LEVEL_WARNING   "W/"
#define KLOG_LEVEL_INFO      "I/"
#define KLOG_LEVEL_VERBOSE   "V/"
#define KLOG_LEVEL_DEBUG     "D/"

#ifndef TAG
#define TAG "kunos"
#endif

//#ifndef LOG_TAG
//#define LOG_TAG "["TAG"]"
//#endif

#ifndef CONFIG_KLOG_LEVEL
#define CONFIG_KLOG_LEVEL OPTION_KLOG_LEVEL_DEFAULT
#endif

#define KLOG(systag, level, fmt, arg...)  \
        syslog(systag, "%s:%s <%s:%u>: " fmt "\n", level, TAG, __func__, __LINE__, ##arg)

#define KTAGLOG(systag, level, tag, fmt, arg...)  \
        syslog(systag, "%s:%s <%s:%u>: " fmt "\n", level, tag, __func__, __LINE__, ##arg)


#define KLOGE(fmt, arg...) KLOG(LOG_ERR, KLOG_LEVEL_ERROR, fmt, ##arg)
#define KTAGLOGE(tag, fmt, arg...) KTAGLOG(LOG_ERR, KLOG_LEVEL_ERROR, tag, fmt, ##arg)

#if CONFIG_KLOG_LEVEL == OPTION_KLOG_LEVEL_CLOSE

#define KLOGD(fmt, arg...)
#define KLOGW(fmt, arg...)
#define KLOGI(fmt, arg...)
#define KLOGV(fmt, arg...)
#define KTAGLOGD(tag, fmt, arg...)
#define KTAGLOGW(tag, fmt, arg...)
#define KTAGLOGI(tag, fmt, arg...)
#define KTAGLOGV(tag, fmt, arg...)

#elif CONFIG_KLOG_LEVEL == OPTION_KLOG_LEVEL_DEBUG

#define KLOGD(fmt, arg...) KLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, fmt, ##arg)
#define KLOGW(fmt, arg...)
#define KLOGI(fmt, arg...)
#define KLOGV(fmt, arg...)
#define KTAGLOGD(tag, fmt, arg...) KTAGLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, tag, fmt, ##arg)
#define KTAGLOGW(tag, fmt, arg...)
#define KTAGLOGI(tag, fmt, arg...)
#define KTAGLOGV(tag, fmt, arg...)

#elif CONFIG_KLOG_LEVEL == OPTION_KLOG_LEVEL_WARNING

#define KLOGD(fmt, arg...) KLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, fmt, ##arg)
#define KLOGW(fmt, arg...) KLOG(LOG_WARNING, KLOG_LEVEL_WARNING, fmt, ##arg)
#define KLOGI(fmt, arg...)
#define KLOGV(fmt, arg...)
#define KTAGLOGD(tag, fmt, arg...) KTAGLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, tag, fmt, ##arg)
#define KTAGLOGW(tag, fmt, arg...) KTAGLOG(LOG_WARNING, KLOG_LEVEL_WARNING, tag, fmt, ##arg)
#define KTAGLOGI(tag, fmt, arg...)
#define KTAGLOGV(tag, fmt, arg...)

#elif CONFIG_KLOG_LEVEL == OPTION_KLOG_LEVEL_DEFAULT

#define KLOGD(fmt, arg...) KLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, fmt, ##arg)
#define KLOGW(fmt, arg...) KLOG(LOG_WARNING, KLOG_LEVEL_WARNING, fmt, ##arg)
#define KLOGI(fmt, arg...) KLOG(LOG_INFO, KLOG_LEVEL_INFO, fmt, ##arg)
#define KLOGV(fmt, arg...)
#define KTAGLOGD(tag, fmt, arg...) KTAGLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, tag, fmt, ##arg)
#define KTAGLOGW(tag, fmt, arg...) KTAGLOG(LOG_WARNING, KLOG_LEVEL_WARNING, tag, fmt, ##arg)
#define KTAGLOGI(tag, fmt, arg...) KTAGLOG(LOG_INFO, KLOG_LEVEL_INFO, tag, fmt, ##arg)
#define KTAGLOGV(tag, fmt, arg...)

#elif CONFIG_KLOG_LEVEL == OPTION_KLOG_LEVEL_DETAIL

#define KLOGD(fmt, arg...) KLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, fmt, ##arg)
#define KLOGW(fmt, arg...) KLOG(LOG_WARNING, KLOG_LEVEL_WARNING, fmt, ##arg)
#define KLOGI(fmt, arg...) KLOG(LOG_INFO, KLOG_LEVEL_INFO, fmt, ##arg)
#define KLOGV(fmt, arg...) KLOG(LOG_INFO, KLOG_LEVEL_VERBOSE, fmt, ##arg)
#define KTAGLOGD(tag, fmt, arg...) KTAGLOG(LOG_DEBUG, KLOG_LEVEL_DEBUG, tag, fmt, ##arg)
#define KTAGLOGW(tag, fmt, arg...) KTAGLOG(LOG_WARNING, KLOG_LEVEL_WARNING, tag, fmt, ##arg)
#define KTAGLOGI(tag, fmt, arg...) KTAGLOG(LOG_INFO, KLOG_LEVEL_INFO, tag, fmt, ##arg)
#define KTAGLOGV(tag, fmt, arg...) KTAGLOG(LOG_INFO, KLOG_LEVEL_VERBOSE, tag, fmt, ##arg)

#endif

#endif /*__TINA_LOG_H__*/
