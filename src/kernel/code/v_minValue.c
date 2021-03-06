/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR PrismTech
 *   Limited, its affiliated companies and licensors. All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
 
#include "v_minValue.h"
#include "v_timedValue.h"

void
v_minValueInit(
    v_minValue *_this)
{
    assert(_this != NULL);
    v_minValueReset(_this);
}

void
v_minValueReset(
    v_minValue *_this)
{
    assert(_this != NULL);
    v_timedValueReset(_this);
    v_timedValueSetValue(_this,0x7FFFFFFF);
}

os_timeW
v_minValueGetTime(
    v_minValue *_this)
{
    assert(_this != NULL);
    return v_timedValueGetTime(_this);
}

c_ulong
v_minValueGetValue(
    v_minValue *_this)
{
    assert(_this != NULL);
    return v_timedValueGetValue(_this);
}

void
v_minValueSetValue(
    v_minValue *_this, 
    c_ulong value)
{
    if (value < v_timedValueGetValue(_this)) {
        v_timedValueSetValue(_this,value);
    }
}

