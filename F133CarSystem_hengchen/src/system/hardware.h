/*
 * hardware.h
 *
 *  Created on: 2024年3月6日
 *      Author: guoxs
 */

#ifndef _SYSTEM_HARDWARE_H_
#define _SYSTEM_HARDWARE_H_

typedef void (*temp_protect_cb)(int num);

namespace sys {
namespace hw {

void init();

void add_temp_protect_cb(temp_protect_cb cb);
void remove_temp_protect_cb(temp_protect_cb cb);
const char *getScreenResolution();
}
}

#endif /* _SYSTEM_HARDWARE_H_ */
