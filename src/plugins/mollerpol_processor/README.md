# mollerpol_processor Plugin

The `mollerpol_processor` consumes the typed hit objects produced by `evio_parser` and writes them to a ROOT file as TTrees and histograms. It also writes a human-readable per-event text summary.

The plugin operates at the **physics event level** — it receives individual `JEvent`s that have already been unfolded by `JEventUnfolder_EVIO` and contain fully decoded detector hits.

---

## Table of Contents

- [Architecture](#architecture)
- [Plugin Initialization](#plugin-initialization)
- [Output Files](#output-files)
- [Data Flow](#data-flow)
- [Configuration Parameters](#configuration-parameters)
- [Example Usage](#example-usage)

---

Common hit headers are provided by
`jana2_common_extensions::evio_common_modules_data_types`;

## Architecture

```
evio_parser plugin
    └─ JEventUnfolder_EVIO
            │  (physics-level JEvents with typed hits)
            ▼
mollerpol_processor plugin
    └─ JEventProcessor_mollerpol
            ├─ ProcessSequential(event)
            │       ├─ reads FADC250WaveformHit objects
            │       ├─ reads FADC250PulseHit objects
	    │       ├─ fills ROOT histogram 'm_pulse_integral_hist'
            │       ├─ reads HelicityDecoderData objects
	    │       ├─ reads HelicityScalerHit objects
	    │       ├─ fills ROOT TTree 'm_helicity_scaler_tree'
            │       ├─ fills ROOT TTree 'm_tree' (General)
            │       └─ writes a text summary
            └─ Finish()
                    └─ writes and closes ROOT file
                    └─ closes text file
```

### `JEventProcessor_mollerpol`

The single processor class in this plugin. It:

- Uses `CallbackStyle::ExpertMode` and `ProcessSequential` — all ROOT writes happen on a single thread, which is required for ROOT thread safety.
- Declares all hit inputs as **optional** with `SetOptional(true)`. This means the processor will not throw an error if a given hit type is absent from an event (e.g. an event with no waveform data).
- Uses JANA2's typed `Input<T>` mechanism to retrieve hits from the `JEvent` by type.

---

## Plugin Initialization

`InitPlugin.cc` registers `JEventProcessor_mollerpol` with the JANA2 application:

```cpp
extern "C" {
    void InitPlugin(JApplication* app) {
        InitJANAPlugin(app);
        app->Add(new JEventProcessor_mollerpol());
    }
}

```

No additional services or parsers are registered by this plugin — it relies entirely on objects inserted into `JEvent` by `evio_parser`.

## Output Files

### ROOT file (`mollerpol_processor.root` by default)

| Object | Class | Description |
|---|---|---|
| `m_tree` | `TTree` | General pulse, waveform, and other module data per event |
| `m_helicity_scaler_tree` | `TTree` | SIS3820 helicity scaler data |
| `m_pulse_integral_hist` | `TH1I` | Distribution of `FADC250PulseHit::integral_sum` values |

### Text file (`mollerpol_processor_hits.txt` by default)

Human-readable per-event dump. Each event block lists counts and field values for every hit type present. Events with no hits at all are omitted.

Example output:

```
Event 67
  Waveform hits: 2
    WF rocid=1 slot=3 chan=0 nsamples=40
    WF rocid=1 slot=3 chan=4 nsamples=40
  Pulse hits: 7
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=0 integral_sum=9466
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=4 integral_sum=10442
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=11 integral_sum=10017
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=12 integral_sum=9730
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=13 integral_sum=25820
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=14 integral_sum=41952
    PULSE rocid=1 slot=3 trigger_time=2237961484 chan=15 integral_sum=41679
  Heldec hit: 1
    n_pattsync=75903, helicity=1, tstable=1, window in pat=0, 0th window in pat hel=1, polarity=0
  No FADCScalerHit objects in this event
  No TIScalerHit objects in this event
  No MPDHit objects in this event
  No VFTDCHit objects in this event
  No HelicityScalerHit objects in this event
  ...	   
```

---

## Data Flow 

This plugin is the **consumer end** of the pipeline:

```
evio_common_modules                     mollerpol_processor
──────────────────────────────────      ─────────────────────────────────────
ModuleParser_FADC::parse()
  → EventHits_FADC::insertIntoEvent()
      → event.Insert(waveforms)    →→→  m_waveform_hits_in()        → m_tree
      → event.Insert(pulses)       →→→  m_pulse_hits_in()           → m_tree, m_pulse_integral_hist
ModuleParser_HelicityDecoder::parse()
  → event.Insert(helicity)         →→→  m_heldec_data_in()          → m_tree
ModuleParser_HelicityScaler::parse()
  → EventHits_FADC::insertIntoEvent()
      → event.Insert(scaler)       →→→  m_helicity_scaler_hits.in() → m_helicity_scaler_tree	

...
```

All hit types consumed by this processor are declared in `JEventProcessor_mollerpol.h` as `Input<T>` members. JANA2 resolves them by type at event processing time.

---

## Configuration Parameters

All parameters are set on the JANA2 command line with `-P<name>=<value>`.

| Parameter | Default | `is_shared` | Description |
|---|---|---|---|
| `ROOT_OUT_FILENAME` | `mollerpol_processor.root` | yes | Path/name of the ROOT output file |
| `TXT_OUT_FILENAME` | `mollerpol_processor_hits.txt` | yes | Path/name of the text hit-summary file |

---

## Example Usage

After sourcing `setup_mollerpol.sh` or `setup_mollerpol.csh`, use the installed JCE wrapper:

```bash
"${JCE_HOME}/scripts/jce.sh" -Pplugins=mollerpol_processor data.evio
```

Produces `mollerpol_processor.root` and `mollerpol_processor_hits.txt` in the current directory.

### Custom ROOT output filename

```bash
"${JCE_HOME}/scripts/jce.sh" -Pplugins=mollerpol_processor -PROOT_OUT_FILENAME=run_042.0.root data.evio
```

### With filtering and custom mapping

```bash
"${JCE_HOME}/scripts/jce.sh" -Pplugins=mollerpol_processor -PFILTER:ENABLE=1 -PFILTER:FILE=config/filter.db -PBANKMAP:FILE=config/mapping.db -PROOT_OUT_FILENAME=run_042_filtered.0.root data.evio
```