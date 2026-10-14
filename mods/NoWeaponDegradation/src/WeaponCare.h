// Decides what to do with one of the player's weapons. Holds no game calls, so it can be tested
// without the game (see test/WeaponCareTest.cpp).
//
// Wear ("degradation" in the game) runs from 0, good, to 1, poor. A weapon can carry permanent
// wear that the game's own cleaning cannot remove; a fully cleaned weapon sits at that value.
#pragma once

// Returned when a weapon's wear should stay as it is.
constexpr float LEAVE_WEAR = -1.0f;

// The game offers "Clean" only on a weapon whose wear is above its permanent wear. This much is
// enough for it, and far below anything else the game's scripts react to.
constexpr float LESSON_WEAR = 0.1f;

// Returns the wear to write to a weapon, or LEAVE_WEAR.
//   lessonRunning: the mission that teaches weapon cleaning is running. It cannot be finished
//                  until the player has cleaned a Double-Barreled Shotgun.
//   isLessonGun:   this weapon is a Double-Barreled Shotgun.
constexpr float wearToWrite(bool lessonRunning, bool isLessonGun, float wear, float permanentWear) {
    if (!lessonRunning) return wear > permanentWear ? permanentWear : LEAVE_WEAR;
    // During the lesson nothing is cleaned, so the shotgun it hands over stays dirty. A shotgun
    // the player owned before arrives spotless, and the game then offers nothing to clean.
    if (isLessonGun && wear <= permanentWear) return permanentWear + LESSON_WEAR;
    return LEAVE_WEAR;
}
