
#ifndef _JEventProcessor_mollerpol_h_
#define _JEventProcessor_mollerpol_h_

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>

#include <fstream>
#include <string>
#include <vector>

#include <JANA/JEventProcessor.h>
#include "EventHits_FADC.h"
#include "FADCScalerHit.h"
#include "TIScalerHit.h"
#include "HelicityDecoderData.h"
#include "MPDHit.h"
#include "VFTDCHit.h"
#include "FADC250HallBPulseIntegralHit.h"
#include "HelicityScalerHit.h" //A.A. 9/21/26

/**
 * @struct WaveformTreeRow
 * @brief Data structure representing one row in the waveform ROOT TTree
 * 
 * Contains the waveform hit information to be stored in ROOT Tree:
 * - slot: FADC250 slot number
 * - chan: Channel number within the slot
 * - waveform: Vector of ADC sample values
 */
struct WaveformTreeRow {
    uint32_t slot;
    uint32_t chan;
    uint32_t rocid;
    std::vector<uint32_t> waveform;
};


/**
 * @class JEventProcessor_mollerpol
 * @brief Main event processor for FADC250 detector data analysis
 * 
 * This processor receives FADC250 detector hits (both waveform and pulse hits)
 * and outputs the data to a ROOT file containing:
 * - Waveform TTree: Channel-by-channel waveform data
 * - Pulse integral histogram: Distribution of pulse integral sums
 * 
 * The output filename can be customized via JANA2 parameters.
 */
class JEventProcessor_mollerpol : public JEventProcessor {

private:
    // Declare Inputs
    Input<FADC250WaveformHit>          m_waveform_hits_in {this}; 
    Input<FADC250PulseHit>             m_pulse_hits_in {this};
    Input<FADCScalerHit>               m_fadc_scaler_hits_in {this};
    Input<TIScalerHit>                 m_ti_scaler_hits_in {this};
    Input<HelicityDecoderData>         m_heldec_data_in {this};
    Input<MPDHit>                      m_mpd_hits_in {this};
    Input<VFTDCHit>                    m_vftdc_hits_in {this};
    Input<HelicityScalerHit>           m_helicity_scaler_hits_in {this}; //A.A. 9/21/26
    Input<FADC250HallBPulseIntegralHit> m_hallb_pulse_integral_hits_in {this};

    /**
     * @brief ROOT output filename parameter
     * 
     * This parameter allows users to specify the ROOT output filename via JANA2 configuration.
     * The parameter constructor takes the following arguments:
     * - owner: Pointer to this component (for parameter registration)
     * - name: "ROOT_OUT_FILENAME" - the parameter name used in configuration files/command line
     * - default_value: "mollerpol_processor.root" - default filename if not specified
     * - description: "Output file name for root data" - help text for the parameter
     * - is_shared: if true, the parameter name is used as-is;  
     *              if false (default), the component's prefix (set in the constructor) is prepended to the name.
     */
    Parameter<std::string> m_root_output_filename {this, "ROOT_OUT_FILENAME", "mollerpol_processor.root", "Output file name for ROOT data", true};

    /**
     * @brief Text output filename parameter
     *
     * Text file where per-event summaries of waveform, pulse, and scaler
     * hits are written.
     */
    Parameter<std::string> m_txt_output_filename {this, "TXT_OUT_FILENAME", "mollerpol_processor_hits.txt", "Output text file name for event hit summaries", true};

    // ROOT Tree variables 
    //Waveform Tree Variables
    std::vector<uint32_t> ev_slot;
    std::vector<uint32_t> ev_chan;
    std::vector<uint32_t> ev_waveform;
    std::vector<uint32_t> ev_rocid;

    //Pulse Tree Variables
    static constexpr int maxChan = 64; //A.A. Test
    uint32_t integral_sum;
    uint32_t coarse_time;
    uint32_t fine_time;
    uint32_t pulse_peak;
    uint32_t ev_timestamp1;
    uint32_t ev_timestamp2;
    ULong64_t ev_trigger_time;
    int number_hit;
    double ev_pedestal_sum[maxChan];
    double ev_pedestal_quality[maxChan];
    double ev_integral_sum[maxChan];
    double ev_coarse_time[maxChan];
    double ev_fine_time[maxChan];
    double ev_pulse_time[maxChan];
    double ev_pulse_peak[maxChan];
    double ev_pulse_slot[maxChan];
    double ev_pulse_chan[maxChan];
    double ev_pulse_rocid[maxChan];
    Int_t nadc_chan = 0; //A.A. Test
   

    // Helicity decoder tree variables (A.A. 10/19/26, 11/9/26)
    double ev_helicity_seed;
    double ev_n_tstable_fall;
    double ev_n_tstable_rise;
    double ev_n_pattsync;
    double ev_n_pairsync;
    double ev_time_tstable_start;
    double ev_time_tstable_end;
    double ev_last_tstable_duration;
    double ev_last_tsettle_duration;
    double ev_trig_tstable;
    double ev_trig_pattsync;
    double ev_trig_pairsync;
    double ev_trig_helicity;
    double ev_trig_pat0_helicity;
    double ev_trig_polarity;
    double ev_trig_pat_count;
    double ev_last32wins_pattsync;
    double ev_last32wins_pairsync;
    double ev_last32wins_helicity;
    double ev_last32wins_pattsync_hel;
   
    //3820 Helicity Scaler Tree Variables. A.A. 9/21/26
    std::vector<uint32_t> ev_3820_rocid;
    std::vector<uint32_t> ev_3820_userbit1;
    std::vector<uint32_t> ev_3820_userbit2;
    std::vector<uint32_t> ev_3820_chan;
    std::vector<uint32_t> ev_3820_scaler_count;
    std::vector<uint32_t> ev_3820_event_num;
    std::vector<uint32_t> ev_3820_event_time;

    // ROOT output objects
    TFile *m_root_output_file;                ///< ROOT file for histogram and tree storage
    WaveformTreeRow m_waveform_tree_row;      ///< Data structure holding the current row for TTree filling
    TTree *m_waveform_tree;                   ///< ROOT tree for waveform data
    TTree *m_tree;                            ///< ROOT tree for physics event
    TH1I *m_pulse_integral_hist;              ///< Histogram of pulse integral sums
    //TTree *m_pulse_tree;                      ///< ROOT tree for pulse hit
    TTree *m_helicity_scaler_tree;            ///< Root tree for SIS3820 helicity scaler data. A.A. 9/21/26
    
    // Text output for human-readable dump of hits per event
    std::ofstream m_txt_output_file;

public:

    JEventProcessor_mollerpol();
    virtual ~JEventProcessor_mollerpol() = default;

    void Init() override;
    void ProcessSequential(const JEvent& event) override;
    void Finish() override;

};

#endif // _JEventProcessor_mollerpol_h_

