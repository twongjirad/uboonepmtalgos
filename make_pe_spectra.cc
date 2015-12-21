#include <iostream>
#include <string>
#include <vector>
#include "TFile.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TTree.h"

class Options {

public:  
  Options() {
    modulename = "specalib";
    NCHANNELS = 32;
    rmsmax = 1.0;
    pelownbins = 50;
    peallnbins = 500;
    pelowmax = 50;
    peallmax = 500;
    setlivetime = -1.0;
    nspertick = 15.625e-9;
    use_livetime_shortcut = false;
  };
  virtual ~Options() {};


  std::string inputfilename;
  std::string outputfilename;
  std::string modulename;
  int pelownbins;
  int peallnbins;
  double pelowmax;
  double peallmax;
  double rmsmax;
  int NCHANNELS;
  double setlivetime;
  double nspertick;
  bool use_livetime_shortcut;
};


void parse_args( int nargs, char** argv, Options& options );

int main( int nargs, char** argv ) {

  std::cout << "Make PE Spectrum" << std::endl;

  Options options;
  parse_args( nargs, argv, options );

  // Open Files
  TFile* input  = new TFile( options.inputfilename.c_str(), "OPEN" );
  TFile* output = new TFile( options.outputfilename.c_str(), "RECREATE" );

  // get channel histograms
  TH1D* hspe[options.NCHANNELS];
  TH1D* hnorm[options.NCHANNELS];
  double spe_amp[options.NCHANNELS];
  double spe_charge[options.NCHANNELS];

  for (int ich=0; ich<options.NCHANNELS; ich++) {
    char histname_ave[100];
    sprintf( histname_ave, "%s/hSPE_ave_femch%02d", options.modulename.c_str(), ich );
    char histname_norm[100];
    sprintf( histname_norm, "%s/hSPE_norm_femch%02d", options.modulename.c_str(), ich );
    hspe[ich] = (TH1D*)input->Get( histname_ave );
    hnorm[ich] = (TH1D*)input->Get( histname_norm );
    // normalize them
    if ( hspe[ich] && hnorm[ich] ) 
      hspe[ich]->Divide( hnorm[ich] );
    else {
      std::cout << "could not load SPE histograms for Channel " << ich << std::endl;
      throw;
    }
    // get amp
    spe_amp[ich] = hspe[ich]->GetMaximum();
    spe_charge[ich] = hspe[ich]->Integral();
  }
  
  // loop over eventtree and sum up number of ticks to get livetime
  TTree* eventtree = (TTree*)input->Get("specalib/eventtree");
  int nsamples = 0;
  eventtree->SetBranchAddress( "nsamples", &nsamples );
  TTree* pulsetree = (TTree*)input->Get("specalib/pulsetree");
  double baselinerms, charge, maxamp;
  int opchannel;
  pulsetree->SetBranchAddress( "baselinerms", &baselinerms );
  pulsetree->SetBranchAddress( "charge", &charge );
  pulsetree->SetBranchAddress( "maxamp", &maxamp );
  pulsetree->SetBranchAddress( "opchannel", &opchannel );

  //Make Output PE histograms
  TH1D* pe_all[options.NCHANNELS];
  TH1D* pe_low[options.NCHANNELS];
  for (int ich=0; ich<options.NCHANNELS; ich++) {
    char histname_all[100];
    sprintf( histname_all, "hpe_all_femch%d", ich );
    pe_all[ich] = new TH1D( histname_all, ";pe;rate (kHz)", options.peallnbins, 0, options.peallmax );
    char histname_low[100];
    sprintf( histname_low, "hpe_low_femch%d", ich );
    pe_low[ich] = new TH1D( histname_low, ";pe;rate (kHz)", options.pelownbins, 0, options.pelowmax );
  }
  TH1D* pe_all_tot = new TH1D("hpe_all_tot",";pe;rate (kHz)", options.peallnbins, 0, options.peallmax );
  TH1D* pe_low_tot = new TH1D("hpe_low_tot",";pe;rate (kHz)", options.pelownbins, 0, options.pelowmax );
 
  unsigned long entry = 0;
  long bytes = pulsetree->GetEntry( entry );
  while ( bytes>0 ) {
    
    if (opchannel>=0 & opchannel<options.NCHANNELS) {

      if ( baselinerms<options.rmsmax && charge>0 ) {
	double pe = charge/spe_charge[opchannel];
	double pe_amp = maxamp/spe_amp[opchannel];
	pe_all[opchannel]->Fill( pe );
	pe_low[opchannel]->Fill( pe );
	pe_all_tot->Fill( pe );
	pe_low_tot->Fill( pe );
      }

    }

    entry++;
    bytes = pulsetree->GetEntry( entry );
  }
  std::cout << "Pulse Tree Entries Procesed: " << entry << std::endl;

  double livetime = 0.0;
  if ( !options.use_livetime_shortcut && options.setlivetime<0.0 ) {
    entry = 0;
    bytes = eventtree->GetEntry( entry );
    while ( bytes>0 ) {
      livetime += (nsamples-1)*options.nspertick;
      entry++;
      bytes = eventtree->GetEntry( entry );
    }
  }
  else if ( options.setlivetime>0.0 ) {
    livetime = options.setlivetime;
  }
  else {
    eventtree->GetEntry(0);
    livetime = (nsamples-1)*options.nspertick*eventtree->GetEntries();
  }
  std::cout << "processed " << entry << " eventtree entries" << std::endl;
  std::cout << "Livetime: " << livetime << " seconds" << std::endl;

  // normalize histograms
  for (int ich=0; ich<options.NCHANNELS; ich++) {
    pe_low[ich]->Scale( 1e-3/livetime );
    pe_all[ich]->Scale( 1e-3/livetime );
  }
  pe_all_tot->Scale( 1e-3/(options.NCHANNELS*livetime) );
  pe_low_tot->Scale( 1e-3/(options.NCHANNELS*livetime) );

  output->Write();

  return 0;

};

void parse_args( int nargs, char** argv, Options& options ) {

  int iarg=1;
  while ( iarg<nargs ) {
    if ( argv[iarg]==std::string("-f") ) {
      options.inputfilename = argv[++iarg];
      std::cout << "setting input file to " << options.inputfilename << std::endl;
    }
    else if ( argv[iarg]==std::string("-o")) {
      options.outputfilename = argv[++iarg];
      std::cout << "setting output file to " << options.outputfilename << std::endl;
    }
    else {
      std::cout << "unrecognized option: " << std::string(argv[iarg]) << std::endl;
      throw;
    }
    iarg++;
  }
}
