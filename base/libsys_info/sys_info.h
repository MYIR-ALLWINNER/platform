/*
 * Copyright (C) 2016 Allwinnertech
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYS_INFO
#define SYS_INFO

int sys_get_chipid(void *id);

/*
 * kernel release : linux release version, eg. 3.10 or 4.9
 * kernel version : compile date, eg. #2 SMP PREEMPT Tue Apr 23 17:39:48 CST 2019
 *
 */
int sys_get_kernel_release(void *release);
int sys_get_kernel_version(void *version);

int sys_get_building_system(void *system);
int sys_get_system_version(void *version);

int sys_get_product_brand(void *brand);
int sys_get_product_name(void *name);
int sys_get_product_device(void *device);
int sys_get_product_model(void *model);
int sys_get_product_manufacturer(void *manufacturer);

#endif  //SYS_INFO
