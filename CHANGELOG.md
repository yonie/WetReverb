# Changelog

All notable changes to WetReverb are recorded here. The published notes for each
release are on the [Releases page](https://github.com/yonie/WetReverb/releases);
this file is the portable copy that travels with the source.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
this project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.1] - 2026-09-30

### Fixed
- Linux: no more crash when the editor opens in Carla, or in any host that hands its event loop over through the plug-in window.
- Linux: the lamp above the selected mode is drawn at full size again.
- UI Zoom now sits directly in the right-click menu, so it also shows in hosts that leave out a plug-in's submenus (Studio One).

### Changed
- The window no longer shows resize arrows it could not act on; UI Zoom changes the size.
- The sound and saved settings are unchanged.

## [1.2.0] - 2026-09-26

### Added
- An Audio Unit for Logic and GarageBand, next to the VST3.
- Mono tracks: all four layouts (mono or stereo in, mono or stereo out). A mono
  input feeds both sides; a mono output is the average of left and right.

### Changed
- The macOS builds are signed and notarised by Apple.
- The sound is unchanged, and saved projects reload with the same settings.
