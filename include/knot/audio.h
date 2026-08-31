// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

/*
오디오엔진 설계하기

오디오는 동시에 실행될수 있어야한다
오디오는 원하는 타이밍에 실행되고, 끝날수 있어야한다.


1. 원하는 타이밍에 재생
오디오는 미리 로드해두자
engine.getAudio().load("file.mp3", "audio_name")
engine.getAudio().load("file.wav", "audio_name1")

engine.getAudio().play("audio_name", "any_group_name", 볼륨, 피치, 루프여부)

audio.stopGroup("group name") (해당 그룹에 속한 모든 오디오 중지)
audio.stop("audio_name") (해당 오디오 이름인 모든 오디오 중지)
audio.stopInGroup("audio_name", "group name") (해당 그룹에 속한 해당 오디오 이름의 오디오만 정지)

하나의 그룹에서는 같은 오디오를 한번만 재생 가능

약간 x축 픽셀과 y축 픽셀 두가지를 전달함으로서 픽셀 딱 하나만 집어낼수 있는것처럼 오디오 그룹과 오디오 이름을 x축 y축처럼 생각하는*/

#pragma once
