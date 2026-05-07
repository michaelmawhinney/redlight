# Magnification Probe

`MagRedProbe` is a Windows-only experimental console executable for checking whether the Magnification API can replace the current gamma-ramp backend.

Manual test steps:

1. Build on Windows x64 and run `MagRedProbe.exe`.
1. Confirm the red-only matrix is applied.
1. Verify black stays black, white becomes pure red, and green/blue content lose their non-red output.
1. Press Enter to restore the previous color matrix and exit.
1. Confirm the display returns to normal after exit.
