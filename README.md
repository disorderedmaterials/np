# ModEx
Data Processing for Modulation Excitation

Provides two  different executables:

## modulation_excitation
Queries event mode data and extracts events that occur during pulses (as defined in the configuration file). Extracted events are histogrammed and outputted to a separate Nexus file.

## partition_events
Queries a specified spectra range in event mode data, outputting a Nexus file containing pulse times (relative to the start of the run), for each spectra.
