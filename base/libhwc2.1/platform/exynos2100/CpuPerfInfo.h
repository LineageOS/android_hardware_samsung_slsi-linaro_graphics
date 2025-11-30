/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef CPU_PERF_INFO_H
#define CPU_PERF_INFO_H

#define CPU_CLUSTER_CNT 3
#define CPU_CLUSTER0_MASK   0xf  /* Little cluster */
#define CPU_CLUSTER1_MASK   0x30 /* Middle cluster */
#define CPU_CLUSTER2_MASK   0xc0 /* Big cluster */

typedef struct perfMap {
    /* cpuIDs means bit map for CPU affinity settings
     * for example, If you want CPU 6,7 then set the value as 0xc0 */
    uint32_t cpuIDs;
    // min_clock
    uint32_t minClock[CPU_CLUSTER_CNT];
    // M2M Capa
    uint32_t m2mCapa;
} perfMap_t;

/* If you need affinity settings for each refresh rates, Write CPU_CLUSTER[x]_MASK definitions
 * for example,
 * - If it's needed to set affinity to Middle and Big clusters, Write "CPU_CLUSTER1_MASK | CPU_CLUSTER2_MASK"
 * - If it's not needed to set affinity, write all MASKs */
static std::map<uint32_t, perfMap> perfTable = {
    {30, {CPU_CLUSTER0_MASK | CPU_CLUSTER1_MASK | CPU_CLUSTER2_MASK, {0, 0, 0}, 24}},
    {60, {CPU_CLUSTER0_MASK | CPU_CLUSTER1_MASK | CPU_CLUSTER2_MASK, {0, 0, 0}, 8}},
    {120, {CPU_CLUSTER0_MASK | CPU_CLUSTER1_MASK | CPU_CLUSTER2_MASK, {0, 0, 0}, 4}},
};

#endif
