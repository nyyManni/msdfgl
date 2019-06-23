# Changelog

## [0.2] - 2019-06-23
### Changes:
- Fixed noise from the atlas texture
- Moved all printf-commands behind one API call
- Bugfixes, error handling improvements and cleanup
- Support using the same texture with multiple fonts
- Static library on Linux/macOS
- Render only one "empty box" for control characters.
### Known issues:
- No support for cubic segments yet
- Color mapping for certain shapes is incomplete
- Rendering calls use a geomety shader
- Small errors in shapes, such as letters `q` and `k` with `LiberationSerif-Regular.ttf`. Visible only with a really large font size.
- DLL build on windows not working
