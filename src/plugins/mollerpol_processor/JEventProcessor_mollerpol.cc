#include "JEventProcessor_mollerpol.h"
#include <JANA/JLogger.h>

/**
 * @brief Constructor for JEventProcessor_mollerpol
 * 
 * Initialize the processor with the appropriate type name, prefix, and callback style.
 */
JEventProcessor_mollerpol::JEventProcessor_mollerpol() {
    SetTypeName(NAME_OF_THIS);                    // Provide JANA with this class's name
    SetPrefix("jeventprocessor_mollerpol");       // Set unique prefix for parameters
    SetCallbackStyle(CallbackStyle::ExpertMode);  // Use expert mode for full control

    // All of these are optional because not all events will have these hits
    m_fadc_scaler_hits_in.SetOptional(true);
    m_pulse_hits_in.SetOptional(true);
    m_waveform_hits_in.SetOptional(true);
    m_ti_scaler_hits_in.SetOptional(true);
    m_heldec_data_in.SetOptional(true);
    m_mpd_hits_in.SetOptional(true);
    m_vftdc_hits_in.SetOptional(true);
    m_helicity_scaler_hits_in.SetOptional(true);
    m_hallb_pulse_integral_hits_in.SetOptional(true);
}

/**
 * @brief Initialize the processor
 * 
 * Called once at the start of processing. Open the output files and set up
 * any necessary resources for event processing.
 */
void JEventProcessor_mollerpol::Init() {
    LOG << "JEventProcessor_mollerpol::Init" << LOG_END;
    
    // Open the ROOT output file
    m_root_output_file = new TFile(m_root_output_filename().c_str(), "RECREATE");
    if (m_root_output_file == nullptr || m_root_output_file->IsZombie()) {
        throw JException("Failed to open ROOT output file: " + m_root_output_filename());  
    }

    // Initialize the waveform tree row data structure
    m_waveform_tree_row = WaveformTreeRow();

    // Create ROOT tree for overall event data
    m_tree = new TTree("m_tree", "Physics Event Tree");

    
    // Create ROOT branches for waveform data.
    m_tree->Branch("wv_slot", &ev_slot);
    m_tree->Branch("wv_chan", &ev_chan);
    m_tree->Branch("waveform", &ev_waveform);
    m_tree->Branch("wv_rocid", &ev_rocid);

    
    // Create ROOT branches for pulse data.
    m_tree->Branch("pl_timestamp1",   &ev_timestamp1); //Lower 24 bits
    m_tree->Branch("pl_timestamp2",   &ev_timestamp2); //Upper 24 bits
    m_tree->Branch("fEvtHdr.fEvtTime",&ev_trigger_time);
    m_tree->Branch("Ndata.M.cal.adc_chan", &nadc_chan,      "Ndata.M.cal.adc_chan/I");
    m_tree->Branch("M.cal.adc",       ev_integral_sum,      "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("M.cal.ped",       ev_pedestal_sum,      "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("M.cal.pedq",      ev_pedestal_quality,  "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("pl_coarse_time",  ev_coarse_time,       "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("pl_fine_time",    ev_fine_time,         "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("M.cal.adc_t",     ev_pulse_time,        "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("M.cal.adc_p",     ev_pulse_peak,        "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("M.cal.adc_chan",  ev_pulse_chan,        "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("pl_nhits",        number_hit,           "[Ndata.M.cal.adc_chan]/D"); //NOT M.cal.nhit! Total # pl in EVENT
    m_tree->Branch("pl_slot",         ev_pulse_slot,        "[Ndata.M.cal.adc_chan]/D");
    m_tree->Branch("pl_rocid",        ev_pulse_rocid,       "[Ndata.M.cal.adc_chan]/D");

    
    // Create the 3820 Helicity Scaler Tree. A.A. 9/21/26
    m_helicity_scaler_tree = new TTree("helicity_scaler_tree","SIS3820 Data (channel, scaler count)");
    m_helicity_scaler_tree->Branch("hs_rocid", &ev_3820_rocid);
    m_helicity_scaler_tree->Branch("userbit1", &ev_3820_userbit1);
    m_helicity_scaler_tree->Branch("userbit2", &ev_3820_userbit2);
    m_helicity_scaler_tree->Branch("hs_chan", &ev_3820_chan);
    m_helicity_scaler_tree->Branch("scaler_count", &ev_3820_scaler_count);
    m_helicity_scaler_tree->Branch("hs_event_num", &ev_3820_event_num); //Not fully implemented
    m_helicity_scaler_tree->Branch("hs_event_time",&ev_3820_event_time); //Not implemented


    //Create the Helicity Decoder Tree Branches
    m_tree->Branch("helicity_seed", &ev_helicity_seed);
    m_tree->Branch("heldecoder.n_tstable_fall", &ev_n_tstable_fall);
    m_tree->Branch("heldecoder.n_tstable_rise", &ev_n_tstable_rise);
    m_tree->Branch("heldecoder.n_patsync", &ev_n_pattsync);
    m_tree->Branch("heldecoder.n_pairsync", &ev_n_pairsync);
    m_tree->Branch("heldecoder.time_tstable_start", &ev_time_tstable_start);
    m_tree->Branch("heldecoder.time_tstable_end", &ev_time_tstable_end);
    m_tree->Branch("heldecoder.tstable_dur", &ev_last_tstable_duration);
    m_tree->Branch("heldecoder.tsettle_dur", &ev_last_tsettle_duration);
    m_tree->Branch("heldecoder.tstable_active", &ev_trig_tstable);
    m_tree->Branch("trig_pattsync", &ev_trig_pattsync);
    m_tree->Branch("trig_pairsync", &ev_trig_pairsync);
    m_tree->Branch("heldecoder.hel_reported", &ev_trig_helicity);
    m_tree->Branch("trig_pat0_helicity", &ev_trig_pat0_helicity);
    m_tree->Branch("heldecoder.patpol_reported", &ev_trig_polarity);
    m_tree->Branch("heldecoder.pat_phase", &ev_trig_pat_count);
    m_tree->Branch("last32wins_pattsync", &ev_last32wins_pattsync);
    m_tree->Branch("last32wins_pairsync", &ev_last32wins_pairsync);
    m_tree->Branch("last32wins_helicity", &ev_last32wins_helicity);
    m_tree->Branch("last32wins_pattsync_hel", &ev_last32wins_pattsync_hel);

    
    // Optionally: Text output file for human-readable hit summaries
    m_txt_output_file.open(m_txt_output_filename().c_str());

    // Create histogram for pulse integral distribution
    m_pulse_integral_hist = new TH1I("h_integral", "Pulse Integral Distribution;Integral Sum;Counts", 100, 0, 1);
    m_pulse_integral_hist->SetCanExtend(TH1::kAllAxes);  // Allow ROOT to automatically extend bins
}

/**
 * @brief Process a single event sequentially
 * 
 * Processes FADC250 detector data for a single event. Fills ROOT tree with
 * waveform data and histogram with pulse integral values. This method is 
 * called for each event in the processing pipeline.
 * 
 * @param event Reference to the JANA2 event to process
 */
void JEventProcessor_mollerpol::ProcessSequential(const JEvent &event) {
    
    // Clear previous event data
    ev_slot.clear();
    ev_chan.clear();
    ev_waveform.clear();
    ev_rocid.clear();
    number_hit = 0;
    

    // FADC250 waveform hits
    for (const auto& waveform_hit : m_waveform_hits_in()) {
        // Fill ROOT tree with waveform data
        m_waveform_tree_row.slot = waveform_hit->slot;
        m_waveform_tree_row.chan = waveform_hit->chan;
        m_waveform_tree_row.rocid = waveform_hit->rocid;
        m_waveform_tree_row.waveform = waveform_hit->waveform;

	size_t waveform_sample_number = m_waveform_tree_row.waveform.size();

	ev_slot.insert(ev_slot.end(), waveform_sample_number, m_waveform_tree_row.slot);
	ev_chan.insert(ev_chan.end(), waveform_sample_number, m_waveform_tree_row.chan);
        ev_rocid.insert(ev_rocid.end(), waveform_sample_number, m_waveform_tree_row.rocid);
	ev_waveform.insert(ev_waveform.end(),  m_waveform_tree_row.waveform.begin(), m_waveform_tree_row.waveform.end());
    }

    // FADC250 pulse hits
    ev_timestamp1 = 0;
    ev_timestamp2 = 0;
    ev_trigger_time = 0;
    nadc_chan = 0;
    int nn=0;
    for (const auto& pulse_hit : m_pulse_hits_in()){
        integral_sum = pulse_hit->integral_sum;
        coarse_time = pulse_hit->coarse_time;
        fine_time = pulse_hit->fine_time;
        pulse_peak = pulse_hit->pulse_peak;
	if(integral_sum!=0){
            nn++;
	    ev_timestamp1 = pulse_hit->timestamp1;
	    ev_timestamp2 = pulse_hit->timestamp2;
	    ev_trigger_time = ((ULong64_t)(pulse_hit->timestamp2) << 24) | (pulse_hit->timestamp1);
	    ev_integral_sum[nadc_chan] = pulse_hit->integral_sum;
            ev_coarse_time[nadc_chan] = pulse_hit->coarse_time;
            ev_fine_time[nadc_chan] = pulse_hit->fine_time;
	    ev_pulse_time[nadc_chan] = ((pulse_hit->coarse_time)*4.0+(pulse_hit->fine_time)*0.0625);
            ev_pulse_peak[nadc_chan] = pulse_hit->pulse_peak;
            ev_pulse_slot[nadc_chan] = pulse_hit->slot;
            ev_pulse_chan[nadc_chan] = pulse_hit->chan;
            ev_pulse_rocid[nadc_chan] = pulse_hit->rocid;
	    ev_pedestal_sum[nadc_chan] = pulse_hit->pedestal_sum;
	    ev_pedestal_quality[nadc_chan] = pulse_hit->pedestal_quality;
	    
	    nadc_chan++;
        }  
        
    }

    number_hit = nn;

    // FADC250 pulse hits
    for (const auto& pulse_hit : m_pulse_hits_in()) {
        // Fill histogram with pulse integral values
        m_pulse_integral_hist->Fill(pulse_hit->integral_sum);
    }


    //Helicity Decoder hits
    ev_helicity_seed = 0;
    ev_n_tstable_fall = 0;
    ev_n_tstable_rise = 0;
    ev_n_pattsync = 0;
    ev_n_pairsync = 0;
    ev_time_tstable_start = 0;
    ev_time_tstable_end = 0;
    ev_last_tstable_duration = 0;
    ev_last_tsettle_duration = 0;
    ev_trig_tstable = 0;
    ev_trig_pattsync = 0;
    ev_trig_pairsync = 0;
    ev_trig_helicity = 0;
    ev_trig_pat0_helicity = 0;
    ev_trig_polarity = 0;
    ev_trig_pat_count = 0;
    ev_last32wins_pattsync = 0;
    ev_last32wins_pairsync = 0;
    ev_last32wins_helicity = 0;
    ev_last32wins_pattsync_hel = 0;
    
    // Since we are strictly assuming 1 hit per event this loop is likely unneeded
    for (const auto& heldec_hit : m_heldec_data_in()) {
       ev_helicity_seed = heldec_hit->helicity_seed;
       ev_n_tstable_fall = heldec_hit->n_tstable_fall;
       ev_n_tstable_rise = heldec_hit->n_tstable_rise;
       ev_n_pattsync = heldec_hit->n_pattsync;
       ev_n_pairsync = heldec_hit->n_pairsync;
       ev_time_tstable_start = heldec_hit->time_tstable_start;
       ev_time_tstable_end = heldec_hit->time_tstable_end;
       ev_last_tstable_duration = heldec_hit->last_tstable_duration;
       ev_last_tsettle_duration = heldec_hit->last_tsettle_duration;
       ev_trig_tstable = heldec_hit->trig_tstable;
       ev_trig_pattsync = heldec_hit->trig_pattsync;
       ev_trig_pairsync = heldec_hit->trig_pairsync;
       ev_trig_helicity = heldec_hit->trig_helicity;
       ev_trig_pat0_helicity = heldec_hit->trig_pat0_helicity;
       ev_trig_polarity = heldec_hit->trig_polarity;
       ev_trig_pat_count = heldec_hit->trig_pat_count;
       ev_last32wins_pattsync = heldec_hit->last32wins_pattsync;
       ev_last32wins_pairsync = heldec_hit->last32wins_pairsync;
       ev_last32wins_helicity = heldec_hit->last32wins_helicity;
       ev_last32wins_pattsync_hel = heldec_hit->last32wins_pattsync_hel;
    }
    
    
    const int Num_Channels = 32;
    const auto& scaler_hits = m_helicity_scaler_hits_in();
    int runningClock = 0; // Unimplemented
    
    for (size_t i = 0; i+Num_Channels <= scaler_hits.size(); i += Num_Channels) {
       ev_3820_rocid.clear();
       ev_3820_userbit1.clear();
       ev_3820_userbit2.clear();
       ev_3820_chan.clear();
       ev_3820_scaler_count.clear();
       ev_3820_event_num.clear();

       for (size_t ch = 0; ch < Num_Channels; ++ch) {
	  const auto& scaler_hit = scaler_hits[i + ch];

	  ev_3820_rocid.push_back(scaler_hit->rocid);
	  ev_3820_userbit1.push_back(scaler_hit->userbit1);
	  ev_3820_userbit2.push_back(scaler_hit->userbit2);
	  ev_3820_chan.push_back(scaler_hit->chan);
	  ev_3820_scaler_count.push_back(scaler_hit->count);
       }

       m_helicity_scaler_tree->Fill();
    }

    m_tree->Fill();

    

    // ------------------------------------------------------------------
    // Optional text dump of hits for this event (waveforms, pulses, scalers)
    // ------------------------------------------------------------------
    if (m_txt_output_file.is_open()) {
        const auto& waveform_hits            = m_waveform_hits_in();
        const auto& pulse_hits               = m_pulse_hits_in();
        const auto& fadc_scaler_hits         = m_fadc_scaler_hits_in();
        const auto& ti_scaler_hits           = m_ti_scaler_hits_in();
        const auto& mpd_hits                 = m_mpd_hits_in();
        const auto& vftdc_hits               = m_vftdc_hits_in();
	const auto& helicity_scaler_hits     = m_helicity_scaler_hits_in();  // A.A. 9/21/26
	const auto& heldec_data              = m_heldec_data_in(); // A.A. 11/12/26
        const auto& hallb_pulse_integral_hits = m_hallb_pulse_integral_hits_in();


        bool have_waveforms              = !waveform_hits.empty();
        bool have_pulses                 = !pulse_hits.empty();
        bool have_fadc_scalers           = !fadc_scaler_hits.empty();
        bool have_ti_scalers             = !ti_scaler_hits.empty();
        bool have_mpd_hits               = !mpd_hits.empty();
        bool have_vftdc_hits             = !vftdc_hits.empty();
	bool have_helicity_scaler_hits   = !helicity_scaler_hits.empty();  // A.A. 9/21/26
	bool have_heldec_data            = !heldec_data.empty(); // A.A. 11/12/26
        bool have_hallb_pulse_integrals  = !hallb_pulse_integral_hits.empty();
        // Only write anything if we have at least one type of hit
        if (have_waveforms || have_pulses || have_fadc_scalers || have_ti_scalers || have_mpd_hits || have_vftdc_hits
            || have_heldec_data || have_helicity_scaler_hits || have_hallb_pulse_integrals) {
            auto event_number = event.GetEventNumber();

            m_txt_output_file << "Event " << event_number << "\n";

            // Waveform summary
            if (have_waveforms) {
                m_txt_output_file << "  Waveform hits: " << waveform_hits.size() << "\n";
                for (const auto& hit : waveform_hits) {
                    m_txt_output_file
                        << "    WF rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " nsamples=" << hit->waveform.size()
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No FADC250 waveform hits in this event\n";
            }

            // Pulse summary
            if (have_pulses) {
                m_txt_output_file << "  Pulse hits: " << pulse_hits.size() << "\n";
                for (const auto& hit : pulse_hits) { //trigger_time new
                    m_txt_output_file
                        << "    PULSE rocid=" << hit->rocid
                        << " slot=" << hit->slot
		        << " trigger_time=" << (((ULong64_t)(hit->timestamp2) << 24) | (hit->timestamp1))
                        << " chan=" << hit->chan
                        << " integral_sum=" << hit->integral_sum
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No FADC250 pulse hits in this event\n";
            }

	    // Helicity Decoder summary
            if (have_heldec_data) {
                m_txt_output_file << "  Heldec hit: " << heldec_data.size() << "\n";
                for (const auto& hit : heldec_data) {
                    m_txt_output_file
                        << "    n_pattsync=" << hit->n_pattsync
			<< ", helicity=" << hit->trig_helicity
                        << ", tstable=" << hit->trig_tstable
			<< ", window in pat=" << hit->trig_pat_count
		        << ", 0th window in pat hel=" << hit->trig_pat0_helicity
                        << ", polarity=" << hit->trig_polarity
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No Helicity Decoder data in this event\n";
            }

            // FADC scaler summary
            if (have_fadc_scalers) {
                m_txt_output_file << "  FADC scaler hits: " << fadc_scaler_hits.size() << "\n";
                for (const auto& hit : fadc_scaler_hits) {
                    m_txt_output_file
                        << "    SCALER rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " ncounts=" << hit->ncounts
                        << " counts=";
                    for (uint32_t i = 0; i < hit->ncounts && i < 16u; ++i) {
                        m_txt_output_file << hit->counts[i];
                        if (i + 1 < hit->ncounts && i + 1 < 16u) {
                            m_txt_output_file << ",";
                        }
                    }
                    m_txt_output_file << "\n";
                }
            } else {
                m_txt_output_file << "  No FADCScalerHit objects in this event\n";
	    }

            // TI scaler summary
            if (have_ti_scalers) {
                m_txt_output_file << "  TI scaler hits: " << ti_scaler_hits.size() << "\n";
                for (const auto& hit : ti_scaler_hits) {
                    m_txt_output_file
                        << "    TISCALER rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " nwords=" << hit->nscalerwords
                        << " live_time=" << hit->live_time
                        << " busy_time=" << hit->busy_time
                        << " ts_inputs_before_busy=" << hit->ts_inputs_before_busy
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No TIScalerHit objects in this event\n";
	    }

            // MPD hit summary
            if (have_mpd_hits) {
                m_txt_output_file << "  MPD hits: " << mpd_hits.size() << "\n";
                for (const auto& hit : mpd_hits) {
                    m_txt_output_file
                        << "    MPD rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " trigger_num=" << hit->trigger_num
                        << " trigger_time=" << hit->trigger_time
                        << " mpd_id=" << (int)hit->mpd_id
                        << " fiber_id=" << (int)hit->fiber_id
                        << " apv_channel=" << (int)hit->apv_channel
                        << " apv_id=" << (int)hit->apv_id
                        << " apv_samples=[";
                    for (int i = 0; i < 6; ++i) {
                        m_txt_output_file << hit->apv_samples[i];
                        if (i < 5) m_txt_output_file << ",";
                    }
                    m_txt_output_file << "]\n";
                }
            } else {
                m_txt_output_file << "  No MPDHit objects in this event\n";
            } 

            // VFTDC hit summary
            if (have_vftdc_hits) {
                m_txt_output_file << "  VFTDC hits: " << vftdc_hits.size() << "\n";
                for (const auto& hit : vftdc_hits) {
                    m_txt_output_file
                        << "    VFTDC rocid=" << hit->rocid
                        << " slot=" << hit->slot
                        << " board_id=" << hit->board_id
                        << " timestamp=" << hit->timestamp
                        << " group_num=" << hit->group_num
                        << " channel_num=" << hit->channel_num
                        << " edge_type=" << hit->edge_type
                        << " coarse_time=" << hit->coarse_time
                        << " fine_time=" << hit->fine_time
                        << " two_ns=" << hit->two_ns
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No VFTDCHit objects in this event\n";
            }

	    // Helicity scaler summary. A.A. 9/21/26
            if (have_helicity_scaler_hits) {
                m_txt_output_file << "  Helicity scaler hits: " << helicity_scaler_hits.size() << "\n";
                for (const auto& hit : helicity_scaler_hits) {
                    m_txt_output_file
                        << "    HELICITY SCALER rocid=" << hit->rocid
                        << " userbit1=" << hit->userbit1
                        << " userbit2=" << hit->userbit2
                        << " chan=" << hit->chan
                        << " scaler_count=" << hit->count
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No HelicityScalerHit objects in this event\n";
            }

            // HallB pulse integral summary
            /*if (have_hallb_pulse_integrals) {
                m_txt_output_file << "  HallB pulse integral hits: " << hallb_pulse_integral_hits.size() << "\n";
                for (const auto& hit : hallb_pulse_integral_hits) {
                    m_txt_output_file
                        << "    HALLB_INTEGRAL slot=" << hit->slot
                        << " chan=" << hit->chan
                        << " pulse_number=" << hit->pulse_number
                        << " pulse_integral=" << hit->pulse_integral
                        << "\n";
                }
            } else {
                m_txt_output_file << "  No HallB pulse integral hits in this event\n";
		}*/

            m_txt_output_file << "\n";
	    }
    }
}

/**
 * @brief Finish processing and cleanup
 * 
 * Called once at the end of processing. Close the output file and perform
 * any necessary cleanup operations.
 */
void JEventProcessor_mollerpol::Finish() {
    LOG << "JEventProcessor_mollerpol::Finish" << LOG_END;

    // Write ROOT objects and close ROOT file
    if (m_root_output_file) {
       m_pulse_integral_hist->Write();  // Save integral histogram to file
       m_tree->Write();
       m_helicity_scaler_tree->Write(); // Save 3820 helicity scaler tree to file. A.A. 9/21/26
       m_root_output_file->Close();     // Close ROOT file
       delete m_root_output_file;       // Free memory
       m_root_output_file = nullptr;
    }

    // Close text output file if open
    if (m_txt_output_file.is_open()) {
        m_txt_output_file.close();
    }
}

