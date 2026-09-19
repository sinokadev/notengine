// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <knot/resources.h>


namespace knot {
struct Tween
{
    Transform target;
    float duration; // ms
};

}

// 일단 duration을 복제한 것에서 델타타임*1000을 뺴고
// duration동안 움직일 떄 델타타임*1000 만큼 움직이면 얼마나 움직였을지 알아내야함(아마도 이걸 보간이라 부르는 듯)
// target에서 원래위치만큼 빼면 이동해야할 거리가 나오는데
// 그러면 그 이동해야할 거리를 duration으로 나누면 1ms에 얼마나 움직어야할지 알수있겠네
// 어.........