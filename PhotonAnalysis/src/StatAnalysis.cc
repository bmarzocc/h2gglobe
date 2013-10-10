#include "../interface/StatAnalysis.h"
#include "Sorters.h"
#include "PhotonReducedInfo.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>

#define PADEBUG 0

using namespace std;

void dumpPhoton(std::ostream & eventListText, int lab,
        LoopAll & l, int ipho, int ivtx, TLorentzVector & phop4, float * pho_energy_array);
void dumpJet(std::ostream & eventListText, int lab, LoopAll & l, int ijet);


// ----------------------------------------------------------------------------------------------------
StatAnalysis::StatAnalysis()  :
    name_("StatAnalysis")
{

    systRange  = 3.; // in units of sigma
    nSystSteps = 1;
    doSystematics = true;
    nVBFDijetJetCategories=2;
    scaleClusterShapes = true;
    dumpAscii = false;
    dumpMcAscii = false;
    unblind = false;
    doMcOptimization = false;

    nVBFCategories   = 0;
    nVHhadCategories = 0;
    nVHhadBtagCategories = 0;
    nVHlepCategories = 0;
    nVHmetCategories = 0;
    nTTHhadCategories = 0;
    nTTHlepCategories = 0;
    nCosThetaCategories = 0;

    nVtxCategories = 0;
    R9CatBoundary = 0.94;

    fillOptTree = false;
    doFullMvaFinalTree = false;
    doSpinAnalysis = false;
    
    runJetsForSpin=false;
    splitwzh=false;
    sigmaMrv=0.;
    sigmaMwv=0.;
}

// ----------------------------------------------------------------------------------------------------
StatAnalysis::~StatAnalysis()
{
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::Term(LoopAll& l)
{

    std::string outputfilename = (std::string) l.histFileName;
    // Make Fits to the data-sets and systematic sets
    std::string postfix=Form("_%dTeV",l.sqrtS);
    l.rooContainer->FitToData("data_pol_model"+postfix,"data_mass");  // Fit to full range of dataset

    //    l.rooContainer->WriteSpecificCategoryDataCards(outputfilename,"data_mass","sig_mass","data_pol_model");
    //    l.rooContainer->WriteDataCard(outputfilename,"data_mass","sig_mass","data_pol_model");
    // mode 0 as above, 1 if want to bin in sub range from fit,

    // Write the data-card for the Combinations Code, needs the output filename, makes binned analysis DataCard
    // Assumes the signal datasets will be called signal_name+"_mXXX"
    //    l.rooContainer->GenerateBinnedPdf("bkg_mass_rebinned","data_pol_model","data_mass",1,50,1); // 1 means systematics from the fit effect only the backgroundi. last digit mode = 1 means this is an internal constraint fit
    //    l.rooContainer->WriteDataCard(outputfilename,"data_mass","sig_mass","bkg_mass_rebinned");

    eventListText.close();
    lep_sync.close();

    std::cout << " nevents " <<  nevents << " " << sumwei << std::endl;

    met_sync.close();

    //  kfacFile->Close();
    //  PhotonAnalysis::Term(l);
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::Init(LoopAll& l)
{
    if(PADEBUG)
        cout << "InitRealStatAnalysis START"<<endl;

    nevents=0., sumwei=0.;
    sumaccept=0., sumsmear=0., sumev=0.;

    met_sync.open ("met_sync.txt");

    //Add btagSF variables taking them from database (Badder)
    if(l.itype[l.current] == -301 || l.itype[l.current] == -501 || l.itype[l.current] == -701 || l.itype[l.current] == -1001 || l.itype[l.current] == -1501){  
     
       int mass_point = abs(l.itype[l.current])-1;

       if(PADEBUG) cerr << "Loading btagSF variables for mass-point: " << mass_point << endl;

       char Name[1000];
       sprintf(Name, "/afs/cern.ch/work/b/bmarzocc/public/RadionAnalysis_DONOTREMOVE/jettxt_Radion%d_RD.txt", mass_point);
       std::string name_JetFlavourFile = std::string(Name);
   
       sprintf(Name, "/afs/cern.ch/work/b/bmarzocc/public/RadionAnalysis_DONOTREMOVE/btagEfficiencies_Radion%d_RD.root", mass_point);
       std::string name_btagEfficienciesFile = std::string(Name);

       std::string name_btagSFFile = std::string("/afs/cern.ch/work/b/bmarzocc/public/RadionAnalysis_DONOTREMOVE/btagSF_22Jan2013Rereco.root");

       jetFlavReader = new JetFlavourReader(name_JetFlavourFile.c_str());
       SFReader = new BtagSFReader(name_btagSFFile.c_str());
       EffReader = new BtagEfficiencyReader(name_btagEfficienciesFile.c_str());
    }

    std::string outputfilename = (std::string) l.histFileName;
    eventListText.open(Form("%s",l.outputTextFileName.c_str()));
    lep_sync.open ("lep_sync.txt");
    //eventListText.open(Form("%s_ascii_events.txt",outputfilename.c_str()));
    FillSignalLabelMap(l);
    //
    // These parameters are set in the configuration file
    std::cout
        << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << "StatAnalysis " << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << "leadEtCut "<< leadEtCut << "\n"
        << "subleadEtCut "<< subleadEtCut << "\n"
        << "doTriggerSelection "<< doTriggerSelection << "\n"
        << "nEtaCategories "<< nEtaCategories << "\n"
        << "nR9Categories "<< nR9Categories << "\n"
        << "nPtCategories "<< nPtCategories << "\n"
        << "doEscaleSyst "<< doEscaleSyst << "\n"
        << "doEresolSyst "<< doEresolSyst << "\n"
        << "doEcorrectionSyst "<< doEcorrectionSyst << "\n"
        << "efficiencyFile " << efficiencyFile << "\n"
        << "doPhotonIdEffSyst "<< doPhotonIdEffSyst << "\n"
        << "doR9Syst "<< doR9Syst << "\n"
        << "doVtxEffSyst "<< doVtxEffSyst << "\n"
        << "doTriggerEffSyst "<< doTriggerEffSyst << "\n"
        << "doKFactorSyst "<< doKFactorSyst << "\n"
        << "doPtSpinSyst "<< doPtSpinSyst << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << std::endl;

    // call the base class initializer
    PhotonAnalysis::Init(l);

    // Avoid reweighing from histo conainer
    for(size_t ind=0; ind<l.histoContainer.size(); ind++) {
        l.histoContainer[ind].setScale(1.);
    }

    diPhoCounter_ = l.countersred.size();
    l.countersred.resize(diPhoCounter_+1);

    // initialize the analysis variables
    nInclusiveCategories_ = nEtaCategories;
    if( nR9Categories != 0 ) nInclusiveCategories_ *= nR9Categories;
    if( nPtCategories != 0 ) nInclusiveCategories_ *= nPtCategories;

    // mva removed cp march 8
    //if( useMVA ) nInclusiveCategories_ = nDiphoEventClasses;

    // CP

    nPhotonCategories_ = nEtaCategories;
    if( nR9Categories != 0 ) nPhotonCategories_ *= nR9Categories;

    nVBFCategories   = ((int)includeVBF)*( (mvaVbfSelection && !multiclassVbfSelection) ? mvaVbfCatBoundaries.size()-1 : nVBFEtaCategories*nVBFDijetJetCategories );
    std::sort(mvaVbfCatBoundaries.begin(),mvaVbfCatBoundaries.end(), std::greater<float>() );
    if (multiclassVbfSelection) {
        std::vector<int> vsize;
        vsize.push_back((int)multiclassVbfCatBoundaries0.size());
        vsize.push_back((int)multiclassVbfCatBoundaries1.size());
        vsize.push_back((int)multiclassVbfCatBoundaries2.size());
        std::sort(vsize.begin(),vsize.end(), std::greater<int>());
        // sanity check: there sould be at least 2 vectors with size==2
        if (vsize[0]<2 || vsize[1]<2 ){
            std::cout << "Not enough category boundaries:" << std::endl;
            std::cout << "multiclassVbfCatBoundaries0 size = " << multiclassVbfCatBoundaries0.size() << endl;
            std::cout << "multiclassVbfCatBoundaries1 size = " << multiclassVbfCatBoundaries1.size() << endl;
            std::cout << "multiclassVbfCatBoundaries2 size = " << multiclassVbfCatBoundaries2.size() << endl;
            assert( 0 );
        }
        nVBFCategories   = vsize[0]-1;
        std::sort(multiclassVbfCatBoundaries0.begin(),multiclassVbfCatBoundaries0.end(), std::greater<float>() );
        std::sort(multiclassVbfCatBoundaries1.begin(),multiclassVbfCatBoundaries1.end(), std::greater<float>() );
        std::sort(multiclassVbfCatBoundaries2.begin(),multiclassVbfCatBoundaries2.end(), std::greater<float>() );
    }

    nVHhadCategories = ((int)includeVHhad)*nVHhadEtaCategories;
    nVHhadBtagCategories =((int)includeVHhadBtag);
    nTTHhadCategories =((int)includeTTHhad);
    nTTHlepCategories =((int)includeTTHlep);

    if(includeVHlep){
        nVHlepCategories = nElectronCategories + nMuonCategories;
    }
    if(includeVHlepPlusMet){
        nVHlepCategories = 2;
    }
    nVHmetCategories = (int)includeVHmet;  //met at analysis step

    nCategories_=(nInclusiveCategories_+nVBFCategories+nVHlepCategories+nVHmetCategories+nVHhadCategories+nVHhadBtagCategories+nTTHhadCategories+nTTHlepCategories); 
    //    nCategories_=(nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories+nVHmetCategories);  //met at analysis step
    //    nCategories_=(nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories);
    if (doSpinAnalysis) nCategories_*=nCosThetaCategories;

    effSmearPars.categoryType = effPhotonCategoryType;
    effSmearPars.n_categories = effPhotonNCat;
    effSmearPars.efficiency_file = efficiencyFile;

    diPhoEffSmearPars.n_categories = 8;
    diPhoEffSmearPars.efficiency_file = efficiencyFile;

    if( doEcorrectionSmear ) {
        // instance of this smearer done in PhotonAnalysis
        photonSmearers_.push_back(eCorrSmearer);
    }
    if( doEscaleSmear ) {
        setupEscaleSmearer();
    }
    if( doEresolSmear ) {
        setupEresolSmearer();
    }
    if( doPhotonIdEffSmear ) {
        // photon ID efficiency
        std::cerr << __LINE__ << std::endl;
        idEffSmearer = new EfficiencySmearer( effSmearPars );
        idEffSmearer->name("idEff");
        idEffSmearer->setEffName("ratioTP");
        idEffSmearer->init();
        idEffSmearer->doPhoId(true);
        photonSmearers_.push_back(idEffSmearer);
    }
    if( doR9Smear ) {
        // R9 re-weighting
        r9Smearer = new EfficiencySmearer( effSmearPars );
        r9Smearer->name("r9Eff");
        r9Smearer->setEffName("ratioR9");
        r9Smearer->init();
        r9Smearer->doR9(true);
        photonSmearers_.push_back(r9Smearer);
    }
    if( doVtxEffSmear ) {
        // Vertex ID
        std::cerr << __LINE__ << std::endl;
        vtxEffSmearer = new DiPhoEfficiencySmearer( diPhoEffSmearPars );   // triplicate TF1's here
        vtxEffSmearer->name("vtxEff");
        vtxEffSmearer->setEffName("ratioVertex");
        vtxEffSmearer->doVtxEff(true);
        vtxEffSmearer->init();
        diPhotonSmearers_.push_back(vtxEffSmearer);
    }
    if( doTriggerEffSmear ) {
        // trigger efficiency
        std::cerr << __LINE__ << std::endl;
        triggerEffSmearer = new DiPhoEfficiencySmearer( diPhoEffSmearPars );
        triggerEffSmearer->name("triggerEff");
        triggerEffSmearer->setEffName("effL1HLT");
        triggerEffSmearer->doVtxEff(false);
        triggerEffSmearer->init();
        diPhotonSmearers_.push_back(triggerEffSmearer);
    }
    if(doKFactorSmear) {
        // kFactor efficiency
        std::cerr << __LINE__ << std::endl;
        kFactorSmearer = new KFactorSmearer( kfacHist, l.normalizer() );
        kFactorSmearer->name("kFactor");
        kFactorSmearer->init();
        genLevelSmearers_.push_back(kFactorSmearer);
    }
    if(doPtSpinSmear) {
        // ptSpin efficiency
        std::cerr << __LINE__ << std::endl;
        ptSpinSmearer = new PtSpinSmearer( ptspinHist, l.normalizer() );
        ptSpinSmearer->name("ptSpin");
        ptSpinSmearer->init();
        genLevelSmearers_.push_back(ptSpinSmearer);
    }
    if(doInterferenceSmear) {
        // interference efficiency
        std::cerr << __LINE__ << std::endl;
        interferenceSmearer = new InterferenceSmearer( l.normalizer(), 2.5e-2,0.);
        genLevelSmearers_.push_back(interferenceSmearer);
    }

    // Define the number of categories for the statistical analysis and
    // the systematic sets to be formed

    // FIXME move these params to config file
    l.rooContainer->SetNCategories(nCategories_);
    l.rooContainer->nsigmas = nSystSteps;
    l.rooContainer->sigmaRange = systRange;

    if( doEcorrectionSmear && doEcorrectionSyst ) {
        // instance of this smearer done in PhotonAnalysis
        systPhotonSmearers_.push_back(eCorrSmearer);
        std::vector<std::string> sys(1,eCorrSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doEscaleSmear && doEscaleSyst ) {
        setupEscaleSyst(l);
        //// systPhotonSmearers_.push_back( eScaleSmearer );
        //// std::vector<std::string> sys(1,eScaleSmearer->name());
        //// std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        //// l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doEresolSmear && doEresolSyst ) {
        setupEresolSyst(l);
        //// systPhotonSmearers_.push_back( eResolSmearer );
        //// std::vector<std::string> sys(1,eResolSmearer->name());
        //// std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        //// l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doPhotonIdEffSmear && doPhotonIdEffSyst ) {
        systPhotonSmearers_.push_back( idEffSmearer );
        std::vector<std::string> sys(1,idEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doR9Smear && doR9Syst ) {
        systPhotonSmearers_.push_back( r9Smearer );
        std::vector<std::string> sys(1,r9Smearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doVtxEffSmear && doVtxEffSyst ) {
        systDiPhotonSmearers_.push_back( vtxEffSmearer );
        std::vector<std::string> sys(1,vtxEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doTriggerEffSmear && doTriggerEffSyst ) {
        systDiPhotonSmearers_.push_back( triggerEffSmearer );
        std::vector<std::string> sys(1,triggerEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if(doKFactorSmear && doKFactorSyst) {
        systGenLevelSmearers_.push_back(kFactorSmearer);
        std::vector<std::string> sys(1,kFactorSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if(doPtSpinSmear && doPtSpinSyst) {
        systGenLevelSmearers_.push_back(ptSpinSmearer);
        std::vector<std::string> sys(1,ptSpinSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }

    // ----------------------------------------------------
    // ----------------------------------------------------
    // Global systematics - Lumi
    l.rooContainer->AddGlobalSystematic("lumi",1.045,1.00);
    // ----------------------------------------------------

    // Create observables for shape-analysis with ranges
    // l.rooContainer->AddObservable("mass" ,100.,150.);
    l.rooContainer->AddObservable("CMS_hgg_mass" ,massMin,massMax);
    l.rooContainer->AddConstant("IntLumi",l.intlumi_);
    l.rooContainer->AddConstant("Sqrts",(double)l.sqrtS);
    
    // SM Model
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
        int sig = sigPointsToBook[isig];
        l.rooContainer->AddConstant(Form("XSBR_ggh_%d",sig),l.normalizer()->GetXsection(double(sig),"ggh")*l.normalizer()->GetBR(double(sig)));
        l.rooContainer->AddConstant(Form("XSBR_vbf_%d",sig),l.normalizer()->GetXsection(double(sig),"vbf")*l.normalizer()->GetBR(double(sig)));
        l.rooContainer->AddConstant(Form("XSBR_wh_%d",sig),l.normalizer()->GetXsection(double(sig),"wh")*l.normalizer()->GetBR(double(sig)));
        l.rooContainer->AddConstant(Form("XSBR_zh_%d",sig),l.normalizer()->GetXsection(double(sig),"zh")*l.normalizer()->GetBR(double(sig)));
        l.rooContainer->AddConstant(Form("XSBR_tth_%d",sig),l.normalizer()->GetXsection(double(sig),"tth")*l.normalizer()->GetBR(double(sig)));
    }

    // -----------------------------------------------------
    // Configurable background model
    // if no configuration was given, set some defaults
    std::string postfix=Form("_%dTeV",l.sqrtS);

    if( bkgPolOrderByCat.empty() ) {
        for(int i=0; i<nCategories_; i++){
            if(i<nInclusiveCategories_) {
                bkgPolOrderByCat.push_back(5);
            } else if(i<nInclusiveCategories_+nVBFCategories){
                bkgPolOrderByCat.push_back(3);
            } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories){
                bkgPolOrderByCat.push_back(2);
            } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories){
                bkgPolOrderByCat.push_back(1);
		}else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHhadBtagCategories+nVHlepCategories+nTTHhadCategories+nTTHlepCategories){
		bkgPolOrderByCat.push_back(3);
            }
        }
    }
    // build the model
    buildBkgModel(l, postfix);
/* //FIXME OLIVIER: CHECK IF IT CRASHES
    // -----------------------------------------------------
    // Make some data sets from the observables to fill in the event loop
    // Binning is for histograms (will also produce unbinned data sets)
    l.rooContainer->CreateDataSet("CMS_hgg_mass","data_mass"    ,nDataBins); // (100,110,150) -> for a window, else full obs range is taken
    l.rooContainer->CreateDataSet("CMS_hgg_mass","bkg_mass"     ,nDataBins);

    // Create Signal DataSets:
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
	int sig = sigPointsToBook[isig];
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV", sig),nDataBins);

        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_rv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_rv",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_rv",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_rv",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_rv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_rv", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm_rv", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV_rv", sig),nDataBins);

        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_wv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_wv",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_wv",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_wv",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_wv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_wv", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm_wv", sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV_wv", sig),nDataBins);
    }

    // Make more datasets representing Systematic Shifts of various quantities
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
	int sig = sigPointsToBook[isig];
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),-1);
        if(!splitwzh) l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),-1);
        else{
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),-1);
        }
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV", sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm", sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV", sig),-1);
    }
*/
    bookSignalModel(l,nDataBins);

    // Make sure the Map is filled
    FillSignalLabelMap(l);

    if(PADEBUG)
        cout << "InitRealStatAnalysis END"<<endl;

    // FIXME book of additional variables
}


// ----------------------------------------------------------------------------------------------------
void StatAnalysis::buildBkgModel(LoopAll& l, const std::string & postfix)
{

    // sanity check
    if( bkgPolOrderByCat.size() != nCategories_ ) {
        std::cout << "Number of categories not consistent with specified background model " << nCategories_ << " " << bkgPolOrderByCat.size() << std::endl;
        assert( 0 );
    }

    l.rooContainer->AddRealVar("CMS_hgg_pol6_0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_4"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_5"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_0"+postfix,"@0*@0","CMS_hgg_pol6_0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_1"+postfix,"@0*@0","CMS_hgg_pol6_1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_2"+postfix,"@0*@0","CMS_hgg_pol6_2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_3"+postfix,"@0*@0","CMS_hgg_pol6_3"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_4"+postfix,"@0*@0","CMS_hgg_pol6_4"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_5"+postfix,"@0*@0","CMS_hgg_pol6_4"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_pol5_0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_4"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_0"+postfix,"@0*@0","CMS_hgg_pol5_0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_1"+postfix,"@0*@0","CMS_hgg_pol5_1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_2"+postfix,"@0*@0","CMS_hgg_pol5_2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_3"+postfix,"@0*@0","CMS_hgg_pol5_3"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_4"+postfix,"@0*@0","CMS_hgg_pol5_4"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_quartic0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic0"+postfix,"@0*@0","CMS_hgg_quartic0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic1"+postfix,"@0*@0","CMS_hgg_quartic1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic2"+postfix,"@0*@0","CMS_hgg_quartic2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic3"+postfix,"@0*@0","CMS_hgg_quartic3"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_quad0"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_quad1"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquad0"+postfix,"@0*@0","CMS_hgg_quad0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquad1"+postfix,"@0*@0","CMS_hgg_quad1"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_cubic0"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_cubic1"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_cubic2"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic0"+postfix,"@0*@0","CMS_hgg_cubic0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic1"+postfix,"@0*@0","CMS_hgg_cubic1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic2"+postfix,"@0*@0","CMS_hgg_cubic2"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_lin0"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modlin0"+postfix,"@0*@0","CMS_hgg_lin0"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_plaw0"+postfix,0.01,-10,10);

    l.rooContainer->AddRealVar("CMS_hgg_exp0"+postfix,-1,-10,0);//exp model

    // prefix for models parameters
    std::map<int,std::string> parnames;
    parnames[1] = "modlin";
    parnames[2] = "modquad";
    parnames[3] = "modcubic";
    parnames[4] = "modquartic";
    parnames[5] = "modpol5_";
    parnames[6] = "modpol6_";
    parnames[-1] = "plaw";
    parnames[-10] = "exp";//exp model

    // map order to categories flags + parameters names
    std::map<int, std::pair<std::vector<int>, std::vector<std::string> > > catmodels;
    // fill the map
    for(int icat=0; icat<nCategories_; ++icat) {
        // get the poly order for this category
        int catmodel = bkgPolOrderByCat[icat];
        std::vector<int> & catflags = catmodels[catmodel].first;
        std::vector<std::string> & catpars = catmodels[catmodel].second;
        // if this is the first time we find this order, build the parameters
        if( catflags.empty() ) {
            assert( catpars.empty() );
            // by default no category has the new model
            catflags.resize(nCategories_, 0);
            std::string & parname = parnames[catmodel];
            if( catmodel > 0 ) {
                for(int iorder = 0; iorder<catmodel; ++iorder) {
                    catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), iorder, +postfix.c_str() ) );
                }
            } else {
                if( catmodel != -1 && catmodel != -10 ) {
                    std::cout << "The only supported negative bkg poly orders are -1 and -10, ie 1-parmeter power law -10 exponential" << std::endl;
                    assert( 0 );
                }
		if(catmodel == -1 ){
                catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), 0, +postfix.c_str() ) );
		}else if(catmodel == -10 ){
                    catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), 0, +postfix.c_str() ) );
                }

            }
        } else if ( catmodel != -1 && catmodel != -10) {
            assert( catflags.size() == nCategories_ && catpars.size() == catmodel );
        }
        // chose category order
        catflags[icat] = 1;
    }

    // now loop over the models and allocate the pdfs
    /// for(size_t imodel=0; imodel<catmodels.size(); ++imodel ) {
    for(std::map<int, std::pair<std::vector<int>, std::vector<std::string> > >::iterator modit = catmodels.begin();
            modit!=catmodels.end(); ++modit ) {
        std::vector<int> & catflags = modit->second.first;
        std::vector<std::string> & catpars = modit->second.second;

        if( modit->first > 0 ) {
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                    "0","CMS_hgg_mass",catpars,70+catpars.size());
            // >= 71 means RooBernstein of order >= 1
        } else if (modit->first == -1){
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                    "0","CMS_hgg_mass",catpars,6);
            // 6 is power law
        }else{
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                                                   "0","CMS_hgg_mass",catpars,1);
            // 1 is exp                                                                                                                                                         
        }
    }
}


// ----------------------------------------------------------------------------------------------------
void StatAnalysis::bookSignalModel(LoopAll& l, Int_t nDataBins) 
{
    // -----------------------------------------------------
    // Make some data sets from the observables to fill in the event loop
    // Binning is for histograms (will also produce unbinned data sets)
    l.rooContainer->CreateDataSet("CMS_hgg_mass","data_mass"    ,nDataBins); // (100,110,150) -> for a window, else full obs range is taken
    l.rooContainer->CreateDataSet("CMS_hgg_mass","bkg_mass"     ,nDataBins);

    // Create Signal DataSets:
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
        int sig = sigPointsToBook[isig];
        // SM datasets
        if (!doSpinAnalysis){
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV",sig),nDataBins);

            if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),nDataBins);
            else{
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),nDataBins);
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),nDataBins);
            }
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV_rv",sig),nDataBins);
            if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_rv",sig),nDataBins);
            else{
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_rv",sig),nDataBins);
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_rv",sig),nDataBins);
            }
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_rv",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV_wv",sig),nDataBins);
            if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_wv",sig),nDataBins);
            else{
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_wv",sig),nDataBins);
                l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_wv",sig),nDataBins);
            }
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_wv",sig),nDataBins);
        }
        // Spin Analysis Datasets
        else {
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_rv",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_wv",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_gg_grav_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_qq_grav_mass_m%d",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_gg_grav_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_qq_grav_mass_m%d_rv",sig),nDataBins);

            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_gg_grav_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_qq_grav_mass_m%d_wv",sig),nDataBins);
        }
    }

    // Make more datasets representing Systematic Shifts of various quantities
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
        int sig = sigPointsToBook[isig];
        if (!doSpinAnalysis){
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_Radion_m%d_8TeV_nm",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_Graviton_m%d_8TeV",sig),-1);
            if(!splitwzh) l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),-1);
            else{
                l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),-1);
                l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),-1);
            }
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),-1);
        }
        else {
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_gg_grav_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_qq_grav_mass_m%d",sig),-1);
        }
    }
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::Analysis(LoopAll& l, Int_t jentry)
{
    if(PADEBUG)
        cout << "Analysis START; cur_type is: " << l.itype[l.current] <<endl;

    int cur_type = l.itype[l.current];
    float weight = l.sampleContainer[l.current_sample_index].weight();
    float sampleweight = l.sampleContainer[l.current_sample_index].weight();

    // Set reRunCiC Only if this is an MC event since scaling of R9 and Energy isn't done at reduction
    if (cur_type==0) {
        l.runCiC=reRunCiCForData;
    } else {
        l.runCiC = true;
    }
    if (l.runZeeValidation) l.runCiC=true;

    // make sure that rho is properly set
    if( run7TeV4Xanalysis ) {
        l.version = 12;
    }
    if( l.version >= 13 && forcedRho < 0. ) {
        l.rho = l.rho_algo1;
    }

    l.FillCounter( "Processed", 1. );
    if( weight <= 0. ) {
        std::cout << "Zero or negative weight " << cur_type << " " << weight << std::endl;
        assert( 0 );
    }
    l.FillCounter( "XSWeighted", weight );
    nevents+=1.;

    //PU reweighting
    double pileupWeight=getPuWeight( l.pu_n, cur_type, &(l.sampleContainer[l.current_sample_index]), jentry == 1, l.run);
    sumwei +=pileupWeight;
    weight *= pileupWeight;
    sumev  += weight;

    assert( weight >= 0. );
    l.FillCounter( "PUWeighted", weight );

    if( jentry % 1000 ==  0 ) {
        std::cout << " nevents " <<  nevents << " sumpuweights " << sumwei << " ratio " << sumwei / nevents
            << " equiv events " << sumev << " accepted " << sumaccept << " smeared " << sumsmear << " "
            <<  sumaccept / sumev << " " << sumsmear / sumaccept
            << std::endl;
    }
    // ------------------------------------------------------------
    //PT-H K-factors
    double gPT = 0;
    TLorentzVector gP4(0,0,0,0);

    if (!l.runZeeValidation && cur_type<0){
        gP4 = l.GetHiggs();
        gPT = gP4.Pt();
        //assert( gP4.M() > 0. );
    }

    //Calculate cluster shape variables prior to shape rescaling
    for (int ipho=0;ipho<l.pho_n;ipho++){
        //// l.pho_s4ratio[ipho]  = l.pho_e2x2[ipho]/l.pho_e5x5[ipho];
        l.pho_s4ratio[ipho] = l.pho_e2x2[ipho]/l.bc_s25[l.sc_bcseedind[l.pho_scind[ipho]]];
        float rr2=l.pho_eseffsixix[ipho]*l.pho_eseffsixix[ipho]+l.pho_eseffsiyiy[ipho]*l.pho_eseffsiyiy[ipho];
        l.pho_ESEffSigmaRR[ipho] = 0.0;
        if(rr2>0. && rr2<999999.) {
            l.pho_ESEffSigmaRR[ipho] = sqrt(rr2);
        }
    }

    // Data driven MC corrections to cluster shape variables and energy resolution estimate
    if (cur_type !=0 && scaleClusterShapes ){
        rescaleClusterVariables(l);
    }
    if( useDefaultVertex ) {
        for(int id=0; id<l.dipho_n; ++id ) {
            l.dipho_vtxind[id] = 0;
        }
    } else if( reRunVtx ) {
        reVertex(l);
    }

    // Re-apply JEC and / or recompute JetID
    if(includeVBF || includeVHhad || includeVHhadBtag || includeTTHhad || includeTTHlep) { postProcessJets(l); }

    // initialize diphoton "selectability" and BDTs for this event
    for(int id=0; id<l.dipho_n; ++id ) {
        l.dipho_sel[id]=false;
        l.dipho_BDT[id]=-10;
    }
    
    // Analyse the event assuming nominal values of corrections and smearings
    float mass, evweight, diphotonMVA;
    int diphoton_id=-1, category=-1;
    bool isCorrectVertex;
    bool storeEvent = false;
    if( AnalyseEvent(l,jentry, weight, gP4, mass,  evweight, category, diphoton_id, isCorrectVertex,diphotonMVA) ) {
        // feed the event to the RooContainer
        FillRooContainer(l, cur_type, mass, diphotonMVA, category, evweight, isCorrectVertex, diphoton_id);
        storeEvent = true;
    }
    
    // Systematics uncertaities for the binned model
    // We re-analyse the event several times for different values of corrections and smearings
    if( cur_type < 0 && doMCSmearing && doSystematics ) {
        
        // fill steps for syst uncertainty study
        float systStep = systRange / (float)nSystSteps;

        float syst_mass, syst_weight, syst_diphotonMVA;
        int syst_category;
        std::vector<double> mass_errors;
        std::vector<double> mva_errors;
        std::vector<double> weights;
        std::vector<int>    categories;

        if (diphoton_id > -1 ) {

            // gen-level systematics, i.e. ggH k-factor for the moment
            for(std::vector<BaseGenLevelSmearer*>::iterator si=systGenLevelSmearers_.begin(); si!=systGenLevelSmearers_.end(); si++){
                mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();

                for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                    if( syst_shift == 0. ) { continue; } // skip the central value
                    syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

                    // re-analyse the event without redoing the event selection as we use nominal values for the single photon
                    // corrections and smearings
                    AnalyseEvent(l, jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id, isCorrectVertex,
				 syst_diphotonMVA, true, syst_shift, true, *si, 0, 0 );

                    AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
                            mass_errors, mva_errors, categories, weights);
                }

                FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights,diphoton_id);
            }

            // di-photon systematics: vertex efficiency and trigger
            for(std::vector<BaseDiPhotonSmearer *>::iterator si=systDiPhotonSmearers_.begin(); si!= systDiPhotonSmearers_.end(); ++si ) {
                mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();

                for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                    if( syst_shift == 0. ) { continue; } // skip the central value
                    syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

                    // re-analyse the event without redoing the event selection as we use nominal values for the single photon
                    // corrections and smearings
                    AnalyseEvent(l,jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id, isCorrectVertex,
				 syst_diphotonMVA, true, syst_shift, true,  0, 0, *si );

                    AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
                            mass_errors, mva_errors, categories, weights);
                }

                FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights, diphoton_id);
            }
        }
	
        int diphoton_id_syst=-1;
	category=-1;
        // single photon level systematics: several
        for(std::vector<BaseSmearer *>::iterator  si=systPhotonSmearers_.begin(); si!= systPhotonSmearers_.end(); ++si ) {
            mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();
        
            for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                if( syst_shift == 0. ) { continue; } // skip the central value
                syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

                // re-analyse the event redoing the event selection this time
                AnalyseEvent(l,jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id_syst, isCorrectVertex,
			     syst_diphotonMVA, true, syst_shift, false,  0, *si, 0 );
		
                AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
                        mass_errors, mva_errors, categories, weights);
            }
	    
            FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights, diphoton_id_syst);
        }
    }

    if(PADEBUG)
        cout<<"myFillHistRed END"<<endl;

    return storeEvent;
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::AnalyseEvent(LoopAll& l, Int_t jentry, float weight, TLorentzVector & gP4,
        float & mass, float & evweight, int & category, int & diphoton_id, bool & isCorrectVertex,
        float &kinematic_bdtout,
        bool isSyst,
        float syst_shift, bool skipSelection,
        BaseGenLevelSmearer *genSys, BaseSmearer *phoSys, BaseDiPhotonSmearer * diPhoSys)
{
    if(PADEBUG) cerr << "Entering StatAnalysis::AnalyseEvent" << endl;
    assert( isSyst || ! skipSelection );

    l.createCS_=createCS;

    int cur_type = l.itype[l.current];
    float sampleweight = l.sampleContainer[l.current_sample_index].weight();
    /// diphoton_id = -1;

    std::pair<int,int> diphoton_index;
    vbfIjet1=-1, vbfIjet2=-1;

    // do gen-level dependent first (e.g. k-factor); only for signal
    genLevWeight=1.;
    if(cur_type!=0 ) {
        applyGenLevelSmearings(genLevWeight,gP4,l.pu_n,cur_type,genSys,syst_shift);
    }

    // event selection
    int muVtx=-1;
    int mu_ind=-1;
    int elVtx=-1;
    int el_ind=-1;

    int leadpho_ind=-1;
    int subleadpho_ind=-1;

    if( ! skipSelection ) {

        // first apply corrections and smearing on the single photons
        smeared_pho_energy.clear(); smeared_pho_energy.resize(l.pho_n,0.);
        smeared_pho_r9.clear();     smeared_pho_r9.resize(l.pho_n,0.);
        smeared_pho_weight.clear(); smeared_pho_weight.resize(l.pho_n,1.);
        applySinglePhotonSmearings(smeared_pho_energy, smeared_pho_r9, smeared_pho_weight, cur_type, l, energyCorrected, energyCorrectedError,
                phoSys, syst_shift);
        

        // Fill CiC efficiency plots for ggH, mH=124
        //fillSignalEfficiencyPlots(weight, l);

        // inclusive category di-photon selection
        // FIXME pass smeared R9
        std::vector<bool> veto_indices;
        veto_indices.clear();
//	    diphoton_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0], false, -1, veto_indices, cicCutLevels );
	    diphoton_id = l.DiphotonCiCSelection(l.phoNOCUTS, l.phoNOCUTS, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0], false, -1, veto_indices, cicCutLevels );


	/*--------------------code to run mva diphot Id and save it to a tree. useful for optimization of VHlep/had and TTH--------*/
	float diphoMVA=-999;
	if(optimizeMVA){
	    //implementing MVA
	    float leadptcut=33.;
	    float subleadptcut=25.;
	    //	    cout<<"[DEBUG]:before"<<diphoton_id<<endl;
	    diphoton_id=l.DiphotonMITPreSelection(bdtTrainingType.c_str(),leadptcut,subleadptcut,-0.2,0, &smeared_pho_energy[0],false,false,-100,0,false);
	    //	    cout<<"[DEBUG]:after"<<diphoton_id<<endl;

	    string bdtTrainingPhilosophy="MIT";

	    //	    cout<<"[DEBUG]: index"<<l.dipho_leadind[diphoton_id]<<" index sublead "<<l.dipho_subleadind[diphoton_id]<<" nphot "<<l.pho_n<<endl;
	    TLorentzVector lead_p4;
	    TLorentzVector sublead_p4;
	    if(l.dipho_leadind[diphoton_id] < l.pho_n){
		lead_p4= l.get_pho_p4(l.dipho_leadind[diphoton_id],0,&smeared_pho_energy[0]);
		sublead_p4= l.get_pho_p4(l.dipho_subleadind[diphoton_id],0,&smeared_pho_energy[0]);
	    }

	    //	    cout<<"[DEBUG]:pt"<<lead_p4.Pt()<<endl;

	    if(lead_p4.Pt()>0.1 && sublead_p4.Pt()>0.1 && l.dipho_leadind[diphoton_id] < l.pho_n && l.dipho_subleadind[diphoton_id]< l.pho_n){
		diphoMVA=getDiphoBDTOutput(l,diphoton_id,  lead_p4, sublead_p4,  bdtTrainingPhilosophy);
	    }
	}

	l.diPhotonBDTOutput=diphoMVA;

	/*--------------------------------------------------------------------------------------*/


        // N-1 plots
        if( ! isSyst ) {
            int diphoton_nm1_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoNOCUTS, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0] );
            if(diphoton_nm1_id>-1) {
                float eventweight = weight * smeared_pho_weight[diphoton_index.first] * smeared_pho_weight[diphoton_index.second] * genLevWeight;
                float myweight=1.;
                if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;
                ClassicCatsNm1Plots(l, diphoton_nm1_id, &smeared_pho_energy[0], eventweight, myweight);
            }
        }

        // Exclusive Modes
	diphotonVBF_id = -1;
	diphotonVHhad_id = -1;
	diphotonVHhadBtag_id = -1;
	diphotonTTHhad_id = -1;
	diphotonTTHlep_id = -1;
	diphotonVHlep_id = -1;
	diphotonVHmet_id = -1; //met at analysis step
        VHmuevent = false;
        VHelevent = false;
        VHlep1event = false;
        VHlep2event = false;
        VHmuevent_cat = 0;
        VHelevent_cat = 0;
        VBFevent = false;
        VHhadevent = false;
	    VHhadBtagevent = false;
	    TTHhadevent = false;
	    TTHlepevent = false;
        VHmetevent = false; //met at analysis step

        // lepton tag
        if(includeVHlep){
            //Add tighter cut on dr to tk
            if(run7TeV4Xanalysis){
                diphotonVHlep_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVHlepCut, subleadEtVHlepCut, 4, false, &smeared_pho_energy[0], true, true );
                if(l.pho_drtotk_25_99[l.dipho_leadind[diphotonVHlep_id]] < 1 || l.pho_drtotk_25_99[l.dipho_subleadind[diphotonVHlep_id]] < 1) diphotonVHlep_id = -1;
                VHmuevent=MuonTag2011(l, diphotonVHlep_id, &smeared_pho_energy[0]);
                VHelevent=ElectronTag2011(l, diphotonVHlep_id, &smeared_pho_energy[0]);
            } else {
                //VHmuevent=MuonTag2012(l, diphotonVHlep_id, &smeared_pho_energy[0],lep_sync);
                //VHelevent=ElectronTag2012(l, diphotonVHlep_id, &smeared_pho_energy[0],lep_sync);
                float eventweight = weight * genLevWeight;
                VHmuevent=MuonTag2012B(l, diphotonVHlep_id, mu_ind, muVtx, VHmuevent_cat, &smeared_pho_energy[0], lep_sync, false, -0.2, eventweight, smeared_pho_weight, !isSyst);
                if(!VHmuevent){
                    VHelevent=ElectronTag2012B(l, diphotonVHlep_id, el_ind, elVtx, VHelevent_cat, &smeared_pho_energy[0], lep_sync, false, -0.2, eventweight, smeared_pho_weight, !isSyst);
                }
            }
        }
	
        if(includeVHlepPlusMet){
            float eventweight = weight * genLevWeight;
            float myweight=1.;
            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;
	          VHLepTag2013(l, diphotonVHlep_id, VHlep1event, VHlep2event, false, mu_ind, muVtx, VHmuevent_cat, el_ind, elVtx, VHelevent_cat, &smeared_pho_energy[0], phoidMvaCut, eventweight, smeared_pho_weight, isSyst);
	      }
	
        //Met tag //met at analysis step
        if(includeVHmet){
            int met_cat=-1;
            if( isSyst) VHmetevent=METTag2012B(l, diphotonVHmet_id, met_cat, &smeared_pho_energy[0], met_sync, false, -0.2, true);
            else        VHmetevent=METTag2012B(l, diphotonVHmet_id, met_cat, &smeared_pho_energy[0], met_sync, false, -0.2, false);
        }

        // VBF+hadronic VH
	    if((includeVBF || includeVHhad|| includeVHhadBtag || includeTTHhad || includeTTHlep || runJetsForSpin)&&l.jet_algoPF1_n>1 && !isSyst /*avoid rescale > once*/) {
            l.RescaleJetEnergy();
        }

        if(includeVBF || runJetsForSpin|| runJetsForSpin) {
//            diphotonVBF_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVBFCut, subleadEtVBFCut, 4,false, &smeared_pho_energy[0], true);
            diphotonVBF_id = l.DiphotonCiCSelection(l.phoNOCUTS, l.phoNOCUTS, leadEtVBFCut, subleadEtVBFCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonVBF_id!=-1){
                float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonVBF_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonVBF_id]] * genLevWeight;
                float myweight=1.;
                if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

                VBFevent= ( run7TeV4Xanalysis ?
                        VBFTag2011(l, diphotonVBF_id, &smeared_pho_energy[0], true, eventweight, myweight) :
                        VBFTag2012(vbfIjet1, vbfIjet2, l, diphotonVBF_id, &smeared_pho_energy[0], true, eventweight, myweight) )
                    ;
                if (runJetsForSpin) VBFevent=false;
            }
        }

        if(includeVHhad) {
            VHhadevent = VHhadronicTag2011(l, diphotonVHhad_id, &smeared_pho_energy[0]);
        }

        if(includeVHhadBtag) {
            VHhadBtagevent = VHhadronicBtag2012(l, diphotonVHhadBtag_id, &smeared_pho_energy[0]);
        }

        if(includeTTHhad) {
	    TTHhadevent = TTHhadronicTag2012(l, diphotonTTHhad_id, &smeared_pho_energy[0]);
	}

        if(includeTTHlep) {
	    TTHlepevent = TTHleptonicTag2012(l, diphotonTTHlep_id, &smeared_pho_energy[0]);
	}

	// priority of analysis: TTH leptonic, TTH hadronic, lepton tag, vbf,vh met, vhhad btag, vh had 0tag, 
	if (includeTTHlep&&TTHlepevent) {
	    diphoton_id = diphotonTTHlep_id;
	} else if(includeVHlep&&VHmuevent){
            diphoton_id = diphotonVHlep_id;
        } else if (includeVHlep&&VHelevent){
            diphoton_id = diphotonVHlep_id;
        } else if (includeVHlepPlusMet&&VHlep1event){
            diphoton_id = diphotonVHlep_id;
        } else if (includeVHlepPlusMet&&VHlep2event){
            diphoton_id = diphotonVHlep_id;
        } else if(includeVBF&&VBFevent) {
            diphoton_id = diphotonVBF_id;
        } else if(includeVHmet&&VHmetevent) {
            diphoton_id = diphotonVHmet_id;
	} else if(includeTTHhad&&TTHhadevent) {
	    diphoton_id = diphotonTTHhad_id;
	} else if(includeVHhadBtag&&VHhadBtagevent) {
	    diphoton_id = diphotonVHhadBtag_id;
        } else if(includeVHhad&&VHhadevent) {
            diphoton_id = diphotonVHhad_id;
        }
        // End exclusive mode selection
    }

    //// std::cout << isSyst << " " << diphoton_id << " " << sumaccept << std::endl;

    // if we selected any di-photon, compute the Higgs candidate kinematics
    // and compute the event category
    if (diphoton_id > -1 ) {
        diphoton_index = std::make_pair( l.dipho_leadind[diphoton_id],  l.dipho_subleadind[diphoton_id] );

        // bring all the weights together: lumi & Xsection, single gammas, pt kfactor
        evweight = weight * smeared_pho_weight[diphoton_index.first] * smeared_pho_weight[diphoton_index.second] * genLevWeight;
        if( ! isSyst ) {
            l.countersred[diPhoCounter_]++;
        }

        TLorentzVector lead_p4, sublead_p4, Higgs;
        float lead_r9, sublead_r9;
        TVector3 * vtx;
       
        // should call this guy once by setting vertex above
        fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);

	
        // apply beamspot reweighting if necessary
        if(reweighBeamspot && cur_type!=0) {
            evweight*=BeamspotReweight(vtx->Z(),((TVector3*)l.gv_pos->At(0))->Z());
        }
        // for spin study
        if (reweighPt && cur_type!=0){
            evweight*=PtReweight(Higgs.Pt(),cur_type);
        }


        // FIXME pass smeared R9
        category = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,nPtCategories,nVtxCategories,l.vtx_std_n);
        mass     = Higgs.M();

        // apply di-photon level smearings and corrections
        int selectioncategory = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,0,nVtxCategories,l.vtx_std_n);
        if( cur_type != 0 && doMCSmearing ) {
            applyDiPhotonSmearings(Higgs, *vtx, selectioncategory, cur_type, *((TVector3*)l.gv_pos->At(0)), evweight, zero_, zero_,
                    diPhoSys, syst_shift);
            isCorrectVertex=(*vtx- *((TVector3*)l.gv_pos->At(0))).Mag() < 1.;
        }

        float ptHiggs = Higgs.Pt();
        //if (cur_type != 0) cout << "vtxAn: " << isCorrectVertex << endl;
        // sanity check
        assert( evweight >= 0. );


        // see if the event falls into an exclusive category
        computeExclusiveCategory(l, category, diphoton_index, Higgs.Pt() );

        // if doing the spin analysis calculate new category
        if (doSpinAnalysis) computeSpinCategory(l, category, lead_p4, sublead_p4);

        // fill control plots and counters
        if( ! isSyst ) {
            l.FillCounter( "Accepted", weight );
            l.FillCounter( "Smeared", evweight );
            sumaccept += weight;
            sumsmear += evweight;
            fillControlPlots(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, diphoton_id,
                    category, isCorrectVertex, evweight, vtx, l, muVtx, mu_ind, elVtx, el_ind );

            if (fillOptTree) {
            fillOpTree(l, lead_p4, sublead_p4, smeared_pho_energy, -2, diphoton_index, diphoton_id, -2, -2, weight, evweight, 
                mass, -1, -1, Higgs, -2, category, VBFevent, myVBF_Mjj, myVBFLeadJPt, 
                myVBFSubJPt, nVBFDijetJetCategories, isSyst, "no-syst");
            }
        }
        // dump BS trees if requested
        if (!isSyst && cur_type!=0 && saveBSTrees_) saveBSTrees(l, evweight,category,Higgs, vtx, (TVector3*)l.gv_pos->At(0));

        // save trees for unbinned datacards
        int inc_cat = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,nPtCategories,nVtxCategories,l.vtx_std_n);
        if (!isSyst && cur_type<0 && saveDatacardTrees_ && TMath::Abs(datacardTreeMass-l.normalizer()->GetMass(cur_type))<0.001) saveDatCardTree(l,cur_type,category, inc_cat, evweight, diphoton_index.first,diphoton_index.second,l.dipho_vtxind[diphoton_id],lead_p4,sublead_p4,true,GetSignalLabel(cur_type,l));

        float vtx_mva  = l.vtx_std_evt_mva->at(diphoton_id);
        float vtxProb   = 1.-0.49*(vtx_mva+1.0); /// should better use this: vtxAna_.setPairID(diphoton_id); vtxAna_.vertexProbability(vtx_mva); PM
        // save trees for IC spin analysis
        if (!isSyst && saveSpinTrees_) saveSpinTree(l,category,evweight,Higgs,lead_p4,sublead_p4,diphoton_index.first,diphoton_index.second,diphoton_id,vtxProb,isCorrectVertex);

        //save vbf trees
        if (!isSyst && cur_type<0 && saveVBFTrees_) saveVBFTree(l,category, evweight, -99.);


        if (dumpAscii && !isSyst && (cur_type==0||dumpMcAscii) && mass>=massMin && mass<=massMax ) {
            // New ascii event list for syncrhonizing MVA Preselection + Diphoton MVA
            eventListText <<"type:"<< cur_type 
                << "    run:"   <<  l.run
                << "    lumi:"  <<  l.lumis
                << "    event:" <<  l.event
                << "    rho:" <<    l.rho
                << "    nvtx:" << l.vtx_std_n
                << "    weight:" << evweight
                // Preselection Lead
                << "    r9_1:"  <<  lead_r9
                << "    sceta_1:"   << (photonInfoCollection[diphoton_index.first]).caloPosition().Eta() 
                << "    hoe_1:" <<  l.pho_hoe[diphoton_index.first]
                << "    sigieie_1:" <<  l.pho_sieie[diphoton_index.first]
                << "    ecaliso_1:" <<  l.pho_ecalsumetconedr03[diphoton_index.first] - 0.012*lead_p4.Et()
                << "    hcaliso_1:" <<  l.pho_hcalsumetconedr03[diphoton_index.first] - 0.005*lead_p4.Et()
                << "    trckiso_1:" <<  l.pho_trksumpthollowconedr03[diphoton_index.first] - 0.002*lead_p4.Et()
                << "    chpfiso2_1:" <<  (*l.pho_pfiso_mycharged02)[diphoton_index.first][l.dipho_vtxind[diphoton_id]] 
                << "    chpfiso3_1:" <<  (*l.pho_pfiso_mycharged03)[diphoton_index.first][l.dipho_vtxind[diphoton_id]] 
                // Preselection SubLead
                << "    r9_2:"  <<  sublead_r9
                << "    sceta_2:"   << (photonInfoCollection[diphoton_index.second]).caloPosition().Eta() 
                << "    hoe_2:" <<  l.pho_hoe[diphoton_index.second]
                << "    sigieie_2:" <<  l.pho_sieie[diphoton_index.second]
                << "    ecaliso_2:" <<  l.pho_ecalsumetconedr03[diphoton_index.second] - 0.012*sublead_p4.Et()
                << "    hcaliso_2:" <<  l.pho_hcalsumetconedr03[diphoton_index.second] - 0.005*sublead_p4.Et()
                << "    trckiso_2:" <<  l.pho_trksumpthollowconedr03[diphoton_index.second] - 0.002*sublead_p4.Et()
                << "    chpfiso2_2:" <<  (*l.pho_pfiso_mycharged02)[diphoton_index.second][l.dipho_vtxind[diphoton_id]] 
                << "    chpfiso3_2:" <<  (*l.pho_pfiso_mycharged03)[diphoton_index.second][l.dipho_vtxind[diphoton_id]] 
                // Diphoton MVA inputs
                << "    ptH:"  <<  ptHiggs 
                << "    phoeta_1:"  <<  lead_p4.Eta() 
                << "    phoeta_2:"  <<  sublead_p4.Eta() 
                << "    bsw:"       <<  beamspotWidth 
                << "    pt_1:"      <<  lead_p4.Pt()
                << "    pt_2:"      <<  sublead_p4.Pt()
                << "    pt_1/m:"      <<  lead_p4.Pt()/mass
                << "    pt_2/m:"      <<  sublead_p4.Pt()/mass
                << "    cosdphi:"   <<  TMath::Cos(lead_p4.Phi() - sublead_p4.Phi()) 
                // Extra
                << "    mgg:"       <<  mass 
                << "    e_1:"       <<  lead_p4.E()
                << "    e_2:"       <<  sublead_p4.E()
                << "    vbfevent:"  <<  VBFevent
                << "    muontag:"   <<  VHmuevent
                << "    electrontag:"<< VHelevent
                << "    mettag:"    <<  VHmetevent
                << "    evcat:"     <<  category
                << "    FileName:"  <<  l.files[l.current]
                // VBF MVA
                << "    vbfmva: "   <<  myVBF_MVA;
            // Vertex MVA
            vtxAna_.setPairID(diphoton_id);
            std::vector<int> & vtxlist = l.vtx_std_ranked_list->at(diphoton_id);
            // Conversions
            PhotonInfo p1 = l.fillPhotonInfos(l.dipho_leadind[diphoton_id], vtxAlgoParams.useAllConversions, 0); // WARNING using default photon energy: it's ok because we only re-do th$
            PhotonInfo p2 = l.fillPhotonInfos(l.dipho_subleadind[diphoton_id], vtxAlgoParams.useAllConversions, 0); // WARNING using default photon energy: it's ok because we only re-do$
            int convindex1 = l.matchPhotonToConversion(diphoton_index.first,vtxAlgoParams.useAllConversions);
            int convindex2 = l.matchPhotonToConversion(diphoton_index.second,vtxAlgoParams.useAllConversions);

            for(size_t ii=0; ii<3; ++ii ) {
                eventListText << "\tvertexId"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxlist[ii] : -1);
            }
            for(size_t ii=0; ii<3; ++ii ) {
                eventListText << "\tvertexMva"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxAna_.mva(vtxlist[ii]) : -2.);
            }
            for(size_t ii=1; ii<3; ++ii ) {
                eventListText << "\tvertexdeltaz"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxAna_.vertexz(vtxlist[ii])-vtxAna_.vertexz(vtxlist[0]) : -999.);
            }
            eventListText << "\tptbal:"   << vtxAna_.ptbal(vtxlist[0])
                << "\tptasym:"  << vtxAna_.ptasym(vtxlist[0])
                << "\tlogspt2:" << vtxAna_.logsumpt2(vtxlist[0])
                << "\tp2conv:"  << vtxAna_.pulltoconv(vtxlist[0])
                << "\tnconv:"   << vtxAna_.nconv(vtxlist[0]);

            //Photon IDMVA inputs
            double pfchargedisobad03=0.;
            for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++) {
                pfchargedisobad03 = l.pho_pfiso_mycharged03->at(diphoton_index.first).at(ivtx) > pfchargedisobad03 ? l.pho_pfiso_mycharged03->at(diphoton_index.first).at(ivtx) : pfchargedisobad03;
            }

            eventListText << "\tscetawidth_1: " << l.pho_etawidth[diphoton_index.first]
                << "\tscphiwidth_1: " << l.sc_sphi[diphoton_index.first]
                << "\tsieip_1: " << l.pho_sieip[diphoton_index.first]
                << "\tbc_e2x2_1: " << l.pho_e2x2[diphoton_index.first]
                << "\tpho_e5x5_1: " << l.bc_s25[l.sc_bcseedind[l.pho_scind[diphoton_index.first]]]
                << "\ts4ratio_1: " << l.pho_s4ratio[diphoton_index.first]
                << "\tpfphotoniso03_1: " << l.pho_pfiso_myphoton03[diphoton_index.first]
                << "\tpfchargedisogood03_1: " << l.pho_pfiso_mycharged03->at(diphoton_index.first).at(vtxlist[0])
                << "\tpfchargedisobad03_1: " << pfchargedisobad03
                << "\teseffsigmarr_1: " << l.pho_ESEffSigmaRR[diphoton_index.first];
            pfchargedisobad03=0.;
            for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++) {
                pfchargedisobad03 = l.pho_pfiso_mycharged03->at(diphoton_index.second).at(ivtx) > pfchargedisobad03 ? l.pho_pfiso_mycharged03->at(diphoton_index.second).at(ivtx) : pfchargedisobad03;
            }

            eventListText << "\tscetawidth_2: " << l.pho_etawidth[diphoton_index.second]
                << "\tscphiwidth_2: " << l.sc_sphi[diphoton_index.second]
                << "\tsieip_2: " << l.pho_sieip[diphoton_index.second]
                << "\tbc_e2x2_2: " << l.pho_e2x2[diphoton_index.second]
                << "\tpho_e5x5_2: " << l.bc_s25[l.sc_bcseedind[l.pho_scind[diphoton_index.second]]]
                << "\ts4ratio_2: " << l.pho_s4ratio[diphoton_index.second]
                << "\tpfphotoniso03_2: " << l.pho_pfiso_myphoton03[diphoton_index.second]
                << "\tpfchargedisogood03_2: " << l.pho_pfiso_mycharged03->at(diphoton_index.second).at(vtxlist[0])
                << "\tpfchargedisobad03_2: " << pfchargedisobad03
                << "\teseffsigmarr_2: " << l.pho_ESEffSigmaRR[diphoton_index.second];


            if (convindex1!=-1) {
                eventListText 
                    << "    convVtxZ1:"  <<  vtxAna_.vtxZFromConv(p1)
                    << "    convVtxdZ1:"  <<  vtxAna_.vtxZFromConv(p1)-vtxAna_.vertexz(vtxlist[0])
                    << "    convRes1:"   << vtxAna_.vtxdZFromConv(p1) 
                    << "    convChiProb1:"  <<  l.conv_chi2_probability[convindex1]
                    << "    convNtrk1:"  <<  l.conv_ntracks[convindex1]
                    << "    convindex1:"  <<  convindex1
                    << "    convvtxZ1:" << ((TVector3*) l.conv_vtx->At(convindex1))->Z()
                    << "    convvtxR1:" << ((TVector3*) l.conv_vtx->At(convindex1))->Perp()
                    << "    convrefittedPt1:" << ((TVector3*) l.conv_refitted_momentum->At(convindex1))->Pt();
            } else {
                eventListText 
                    << "    convVtxZ1:"  <<  -999
                    << "    convVtxdZ1:"  <<  -999
                    << "    convRes1:"    <<  -999
                    << "    convChiProb1:"  <<  -999
                    << "    convNtrk1:"  <<  -999
                    << "    convindex1:"  <<  -999
                    << "    convvtxZ1:" << -999
                    << "    convvtxR1:" << -999
                    << "    convrefittedPt1:" << -999;
            }
            if (convindex2!=-1) {
                eventListText 
                    << "    convVtxZ2:"  <<  vtxAna_.vtxZFromConv(p2)
                    << "    convVtxdZ2:"  <<  vtxAna_.vtxZFromConv(p2)-vtxAna_.vertexz(vtxlist[0])
                    << "    convRes2:"   << vtxAna_.vtxdZFromConv(p2)
                    << "    convChiProb2:"  <<  l.conv_chi2_probability[convindex2]
                    << "    convNtrk2:"  <<  l.conv_ntracks[convindex2]
                    << "    convindex2:"  <<  convindex2
                    << "    convvtxZ2:" << ((TVector3*) l.conv_vtx->At(convindex2))->Z()
                    << "    convvtxR2:" << ((TVector3*) l.conv_vtx->At(convindex2))->Perp()
                    << "    convrefittedPt2:" << ((TVector3*) l.conv_refitted_momentum->At(convindex2))->Pt();
            } else {
                eventListText 
                    << "    convVtxZ2:"  <<  -999
                    << "    convVtxdZ2:"  <<  -999
                    << "    convRes2:"    <<  -999
                    << "    convChiProb2:"  <<  -999
                    << "    convNtrk2:"  <<  -999
                    << "    convindex2:"  <<  -999
                    << "    convvtxZ2:" << -999
                    << "    convvtxR2:" << -999
                    << "    convrefittedPt2:" << -999;
            }

            if(VHelevent){
                TLorentzVector* myel = (TLorentzVector*) l.el_std_p4->At(el_ind);
                TLorentzVector* myelsc = (TLorentzVector*) l.el_std_sc->At(el_ind);
                float thiseta = fabs(myelsc->Eta());
                double Aeff=0.;
                if(thiseta<1.0)                   Aeff=0.135;
                if(thiseta>=1.0 && thiseta<1.479) Aeff=0.168;
                if(thiseta>=1.479 && thiseta<2.0) Aeff=0.068;
                if(thiseta>=2.0 && thiseta<2.2)   Aeff=0.116;
                if(thiseta>=2.2 && thiseta<2.3)   Aeff=0.162;
                if(thiseta>=2.3 && thiseta<2.4)   Aeff=0.241;
                if(thiseta>=2.4)                  Aeff=0.23;
                float thisiso=l.el_std_pfiso_charged[el_ind]+std::max(l.el_std_pfiso_neutral[el_ind]+l.el_std_pfiso_photon[el_ind]-l.rho*Aeff,0.);

                TLorentzVector elpho1=*myel + lead_p4;
                TLorentzVector elpho2=*myel + sublead_p4;

                eventListText 
                    << "    elind:"<<       el_ind
                    << "    elpt:"<<        myel->Pt()
                    << "    eleta:"<<       myel->Eta()
                    << "    elsceta:"<<     myelsc->Eta()
                    << "    elmva:"<<       l.el_std_mva_nontrig[el_ind]
                    << "    eliso:"<<       thisiso
                    << "    elisoopt:"<<    thisiso/myel->Pt()
                    << "    elAeff:"<<      Aeff
                    << "    eldO:"<<        fabs(l.el_std_D0Vtx[el_ind][elVtx])
                    << "    eldZ:"<<        fabs(l.el_std_DZVtx[el_ind][elVtx])
                    << "    elmishits:"<<   l.el_std_hp_expin[el_ind]
                    << "    elconv:"<<      l.el_std_conv[el_ind]
                    << "    eldr1:"<<       myel->DeltaR(lead_p4)
                    << "    eldr2:"<<       myel->DeltaR(sublead_p4)
                    << "    elmeg1:"<<      elpho1.M()
                    << "    elmeg2:"<<      elpho2.M();

            } else {
                eventListText 
                    << "    elind:"<<       -1
                    << "    elpt:"<<        -1
                    << "    eleta:"<<       -1
                    << "    elsceta:"<<     -1
                    << "    elmva:"<<       -1
                    << "    eliso:"<<       -1
                    << "    elisoopt:"<<    -1
                    << "    elAeff:"<<      -1
                    << "    eldO:"<<        -1
                    << "    eldZ:"<<        -1
                    << "    elmishits:"<<   -1
                    << "    elconv:"<<      -1
                    << "    eldr1:"<<       -1
                    << "    eldr2:"<<       -1
                    << "    elmeg1:"<<      -1
                    << "    elmeg2:"<<      -1;
            }

            if(VHmetevent){
                TLorentzVector myMet = l.METCorrection2012B(lead_p4, sublead_p4); 
                float corrMet    = myMet.Pt();
                float corrMetPhi = myMet.Phi();

                eventListText 
                    << "    metuncor:"<<        l.met_pfmet
                    << "    metphiuncor:"<<     l.met_phi_pfmet
                    << "    metcor:"<<          corrMet
                    << "    metphicor:"<<       corrMetPhi;
            } else {
                eventListText 
                    << "    metuncor:"<<        -1
                    << "    metphiuncor:"<<     -1
                    << "    metcor:"<<          -1
                    << "    metphicor:"<<       -1;
            }

            if( VBFevent ) {
                eventListText 
                    << "\tjetPt1:"  << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1) )->Pt()
                    << "\tjetPt2:"  << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2) )->Pt()
                    << "\tjetEta1:" << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1) )->Eta()
                    << "\tjetEta2:" << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2) )->Eta()
                    ;
                dumpJet(eventListText,1,l,vbfIjet1);
                dumpJet(eventListText,2,l,vbfIjet2);
            }

            eventListText << endl;
        }

	//useful for TTH and vhhad
	//apply btag SF and (if needed) shifting one sigma for systematics

	//if needed you can compute btag efficiency 
	//	computeBtagEff(l);


	if(includeTTHlep || includeVHhad){
	    bool isMC = l.itype[l.current]!=0;
	    if(isMC && applyBtagSF ){
		if (category==9 ||category ==10){//tth categories
		    evweight*=BtagReweight(l,shiftBtagEffUp_bc,shiftBtagEffDown_bc,shiftBtagEffUp_l,shiftBtagEffDown_l,1);
		}else if (category == 11){//vh categories. loose wp for btag
		    evweight*=BtagReweight(l,shiftBtagEffUp_bc,shiftBtagEffDown_bc,shiftBtagEffUp_l,shiftBtagEffDown_l,0);
		}
	    }
	}
        return (category >= 0 && mass>=massMin && mass<=massMax);
    }

    if(PADEBUG) cerr << "Leaving PhotonAnalysis::FindRadionObjects" << endl;
    return false;
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::FillRooContainer(LoopAll& l, int cur_type, float mass, float diphotonMVA,
        int category, float weight, bool isCorrectVertex, int diphoton_id)
{
    // Fill full mva trees
    if (doFullMvaFinalTree){
        int lead_ind = l.dipho_leadind[diphoton_id];
        int sublead_ind = l.dipho_subleadind[diphoton_id];
        if (PADEBUG) cout << "---------------" << endl;
        if (PADEBUG) cout << "Filling nominal vals" << endl;
        l.FillTree("mass",mass,"full_mva_trees");
        l.FillTree("bdtoutput",diphotonMVA,"full_mva_trees");
        l.FillTree("category",category,"full_mva_trees");
        l.FillTree("weight",weight,"full_mva_trees");
        /* -- for cut based variant --
           l.FillTree("lead_r9",l.pho_r9[lead_ind],"full_mva_trees");
           l.FillTree("sublead_r9",l.pho_r9[sublead_ind],"full_mva_trees");
           l.FillTree("lead_eta",((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
           l.FillTree("sublead_eta",((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
           l.FillTree("lead_phi",((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
           l.FillTree("sublead_phi",((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
         */
    }

    if (cur_type == 0 ) {
        l.rooContainer->InputDataPoint("data_mass",category,mass);
    } else if (cur_type > 0 ) {
        if( doMcOptimization ) {
            l.rooContainer->InputDataPoint("data_mass",category,mass,weight);
        } else {
            l.rooContainer->InputDataPoint("bkg_mass",category,mass,weight);
        }
    } else if (cur_type < 0) {
        l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l),category,mass,weight);
        if (isCorrectVertex) l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l)+"_rv",category,mass,weight);
        else l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l)+"_wv",category,mass,weight);
    }
    //if( category>=0 && fillOptTree ) {
    //l.FillTree("run",l.run);
    //l.FillTree("lumis",l.lumis);
    //l.FillTree("event",l.event);
    //l.FillTree("category",category);
    //l.FillTree("vbfMVA",myVBF_MVA);
    //l.FillTree("vbfMVA0",myVBF_MVA0);
    //l.FillTree("vbfMVA1",myVBF_MVA1);
    //l.FillTree("vbfMVA2",myVBF_MVA2);
    ///// l.FillTree("VBFevent", VBFevent);
    //if( vbfIjet1 > -1 && vbfIjet2 > -1 && ( myVBF_MVA > -2. ||  myVBF_MVA0 > -2 || myVBF_MVA1 > -2 || myVBF_MVA2 > -2 || VBFevent ) ) {
    //    TLorentzVector* jet1 = (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1);
    //    TLorentzVector* jet2 = (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2);

    //    l.FillTree("leadJPt", myVBFLeadJPt);
    //    l.FillTree("subleadJPt", myVBFSubJPt);
    //    l.FillTree("leadJEta", (float)jet1->Eta());
    //    l.FillTree("subleadJEta", (float)jet2->Eta());
    //    l.FillTree("leadJPhi", (float)jet1->Phi());
    //    l.FillTree("subleadJPhi", (float)jet2->Phi());
    //    l.FillTree("MJJ", myVBF_Mjj);
    //    l.FillTree("deltaEtaJJ", myVBFdEta);
    //    l.FillTree("Zep", myVBFZep);
    //    l.FillTree("deltaPhiJJGamGam", myVBFdPhi);
    //    l.FillTree("MGamGam", myVBF_Mgg);
    //    l.FillTree("diphoPtOverM", myVBFDiPhoPtOverM);
    //    l.FillTree("leadPtOverM", myVBFLeadPhoPtOverM);
    //    l.FillTree("subleadPtOverM", myVBFSubPhoPtOverM);
    //    l.FillTree("leadJEta",myVBF_leadEta);
    //    l.FillTree("subleadJEta",myVBF_subleadEta);
    //    l.FillTree("VBF_Pz", myVBF_Pz);
    //    l.FillTree("VBF_S", myVBF_S);
    //    l.FillTree("VBF_K1", myVBF_K1);
    //    l.FillTree("VBF_K2", myVBF_K2);

    //    l.FillTree("deltaPhiGamGam", myVBF_deltaPhiGamGam);
    //    l.FillTree("etaJJ", myVBF_etaJJ);
    //    l.FillTree("VBFSpin_Discriminant", myVBFSpin_Discriminant);
    //    l.FillTree("deltaPhiJJ",myVBFSpin_DeltaPhiJJ);
    //    l.FillTree("cosThetaJ1", myVBFSpin_CosThetaJ1);
    //    l.FillTree("cosThetaJ2", myVBFSpin_CosThetaJ2);
    //    // Small deflection
    //    l.FillTree("deltaPhiJJS",myVBFSpin_DeltaPhiJJS);
    //    l.FillTree("cosThetaS", myVBFSpin_CosThetaS);
    //    // Large deflection
    //    l.FillTree("deltaPhiJJL",myVBFSpin_DeltaPhiJJL);
    //    l.FillTree("cosThetaL", myVBFSpin_CosThetaL);
    //}
    //l.FillTree("sampleType",cur_type);
    ////// l.FillTree("isCorrectVertex",isCorrectVertex);
    ////// l.FillTree("metTag",VHmetevent);
    ////// l.FillTree("eleTag",VHelevent);
    ////// l.FillTree("muTag",VHmuevent);

    //TLorentzVector lead_p4, sublead_p4, Higgs;
    //float lead_r9, sublead_r9;
    //TVector3 * vtx;
    //fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);
    //l.FillTree("leadPt",(float)lead_p4.Pt());
    //l.FillTree("subleadPt",(float)sublead_p4.Pt());
    //l.FillTree("leadEta",(float)lead_p4.Eta());
    //l.FillTree("subleadEta",(float)sublead_p4.Eta());
    //l.FillTree("leadPhi",(float)lead_p4.Phi());
    //l.FillTree("subleadPhi",(float)sublead_p4.Phi());
    //l.FillTree("leadR9",lead_r9);
    //l.FillTree("subleadR9",sublead_r9);
    //l.FillTree("sigmaMrv",sigmaMrv);
    //l.FillTree("sigmaMwv",sigmaMwv);
    //l.FillTree("leadPhoEta",(float)lead_p4.Eta());
    //l.FillTree("subleadPhoEta",(float)sublead_p4.Eta());


    //vtxAna_.setPairID(diphoton_id);
    //float vtxProb = vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n);
    //float altMass = 0.;
    //if( l.vtx_std_n > 1 ) {
    //    int altvtx = (*l.vtx_std_ranked_list)[diphoton_id][1];
    //    altMass = ( l.get_pho_p4( l.dipho_leadind[diphoton_id], altvtx, &smeared_pho_energy[0]) +
    //		l.get_pho_p4( l.dipho_subleadind[diphoton_id], altvtx, &smeared_pho_energy[0]) ).M();
    //}
    //l.FillTree("altMass",altMass);
    //l.FillTree("vtxProb",vtxProb);
    //}
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::AccumulateSyst(int cur_type, float mass, float diphotonMVA,
        int category, float weight,
        std::vector<double> & mass_errors,
        std::vector<double> & mva_errors,
        std::vector<int>    & categories,
        std::vector<double> & weights)
{
    categories.push_back(category);
    mass_errors.push_back(mass);
    mva_errors.push_back(diphotonMVA);
    weights.push_back(weight);
}


// ----------------------------------------------------------------------------------------------------
void StatAnalysis::FillRooContainerSyst(LoopAll& l, const std::string &name, int cur_type,
        std::vector<double> & mass_errors, std::vector<double> & mva_errors,
        std::vector<int>    & categories, std::vector<double> & weights, int diphoton_id)
{
    if (cur_type < 0){
        // fill full mva trees
        if (doFullMvaFinalTree){
            assert(mass_errors.size()==2 && mva_errors.size()==2 && weights.size()==2 && categories.size()==2);
            int lead_ind = l.dipho_leadind[diphoton_id];
            int sublead_ind = l.dipho_subleadind[diphoton_id];
            if (PADEBUG) cout << "Filling template models " << name << endl;
            l.FillTree(Form("mass_%s_Down",name.c_str()),mass_errors[0],"full_mva_trees");
            l.FillTree(Form("mass_%s_Up",name.c_str()),mass_errors[1],"full_mva_trees");
            l.FillTree(Form("bdtoutput_%s_Down",name.c_str()),mva_errors[0],"full_mva_trees");
            l.FillTree(Form("bdtoutput_%s_Up",name.c_str()),mva_errors[1],"full_mva_trees");
            l.FillTree(Form("weight_%s_Down",name.c_str()),weights[0],"full_mva_trees");
            l.FillTree(Form("weight_%s_Up",name.c_str()),weights[1],"full_mva_trees");
            l.FillTree(Form("category_%s_Down",name.c_str()),categories[0],"full_mva_trees");
            l.FillTree(Form("category_%s_Up",name.c_str()),categories[1],"full_mva_trees");
            /* -- for cut based variant -- 
               l.FillTree(Form("lead_r9_%s_Down",name.c_str()),l.pho_r9[lead_ind],"full_mva_trees");
               l.FillTree(Form("lead_r9_%s_Up",name.c_str()),l.pho_r9[lead_ind],"full_mva_trees");
               l.FillTree(Form("sublead_r9_%s_Down",name.c_str()),l.pho_r9[sublead_ind],"full_mva_trees");
               l.FillTree(Form("sublead_r9_%s_Up",name.c_str()),l.pho_r9[sublead_ind],"full_mva_trees");
               l.FillTree(Form("lead_eta_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
               l.FillTree(Form("lead_eta_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
               l.FillTree(Form("sublead_eta_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
               l.FillTree(Form("sublead_eta_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
               l.FillTree(Form("lead_phi_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
               l.FillTree(Form("lead_phi_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
               l.FillTree(Form("sublead_phi_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
               l.FillTree(Form("sublead_phi_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
             */
        }
        // feed the modified signal model to the RooContainer
        l.rooContainer->InputSystematicSet("sig_"+GetSignalLabel(cur_type, l),name,categories,mass_errors,weights);
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::computeExclusiveCategory(LoopAll & l, int & category, std::pair<int,int> diphoton_index, float pt, float diphobdt_output, bool mvaselection)
{
    if(TTHlepevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories +  nVHlepCategories +  nVHmetCategories;
    } else if(VHmuevent || VHlep1event) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories;
        if(nMuonCategories>1) category+=VHmuevent_cat;
    } else if(VHelevent || VHlep2event) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + nMuonCategories;
        if(nElectronCategories>1) category+=VHelevent_cat;
    } else if(VBFevent) {
        category=nInclusiveCategories_;
        if(combinedmvaVbfSelection) {
            int vbfcat=-1;
            vbfcat=categoryFromBoundaries2D(multiclassVbfCatBoundaries0,multiclassVbfCatBoundaries1,multiclassVbfCatBoundaries2,
                                            myVBF_MVA,                  myVBFcombined,              1);
            category += vbfcat;
        } else if( mvaVbfSelection ) {
            if (!multiclassVbfSelection) {
                category += categoryFromBoundaries(mvaVbfCatBoundaries, myVBF_MVA);
            } else if ( vbfVsDiphoVbfSelection ) {
                category += categoryFromBoundaries2D(multiclassVbfCatBoundaries0, multiclassVbfCatBoundaries1, multiclassVbfCatBoundaries2, myVBF_MVA, diphobdt_output, 1.);
            } else {
                category += categoryFromBoundaries2D(multiclassVbfCatBoundaries0, multiclassVbfCatBoundaries1, multiclassVbfCatBoundaries2, myVBF_MVA0, myVBF_MVA1, myVBF_MVA2);
            }
        } else {
            category += l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVBFEtaCategories,1,1)
                + nVBFEtaCategories*l.DijetSubCategory(myVBF_Mjj,myVBFLeadJPt,myVBFSubJPt,nVBFDijetJetCategories);
        }
    } else if(VHmetevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories +  nVHlepCategories;
        if(nVHmetCategories>1) category+=VHmetevent_cat;
    } else if(TTHhadevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories +  nVHlepCategories + nVHmetCategories+nTTHlepCategories;
        if(PADEBUG) cout<<"TTHhad: "<<category<<endl;
    }else if(VHhadBtagevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories +  nVHlepCategories + nVHmetCategories + nTTHlepCategories + nTTHhadCategories;
    } else if(VHhadevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories +  nVHlepCategories + nVHmetCategories + nTTHlepCategories + nTTHhadCategories+nVHhadBtagCategories;
    }
}

void StatAnalysis::computeSpinCategory(LoopAll &l, int &category, TLorentzVector lead_p4, TLorentzVector sublead_p4){

    double cosTheta;
    int cosThetaCategory=-1;
    if (cosThetaDef=="CS"){
        cosTheta = getCosThetaCS(lead_p4,sublead_p4,l.sqrtS);
    }
    else if (cosThetaDef=="HX"){
        cosTheta = getCosThetaHX(lead_p4,sublead_p4,l.sqrtS);
    }
    else {
        cout << "ERROR -- cosThetaDef - " << cosThetaDef << " not recognised" << endl;
        exit(1);
    }

    if (cosThetaCatBoundaries.size()!=nCosThetaCategories+1){
        cout << "ERROR - cosThetaCatBoundaries size does not correspond to nCosThetaCategories" << endl;
        exit(1);
    }

    for (int scat=0; scat<nCosThetaCategories; scat++){
        if (TMath::Abs(cosTheta)>=cosThetaCatBoundaries[scat] && TMath::Abs(cosTheta)<cosThetaCatBoundaries[scat+1]) cosThetaCategory=scat; 
    }

    if (cosThetaCategory==-1) category=-1;
    else category = (category*nCosThetaCategories)+cosThetaCategory;
}

// ----------------------------------------------------------------------------------------------------

void StatAnalysis::fillControlPlots(const TLorentzVector & lead_p4, const  TLorentzVector & sublead_p4, const TLorentzVector & Higgs,
        float lead_r9, float sublead_r9, int diphoton_id,
        int category, bool isCorrectVertex, float evweight, TVector3* vtx, LoopAll & l,
        int muVtx, int mu_ind, int elVtx, int el_ind, float diphobdt_output)
{
    int cur_type = l.itype[l.current];
    float mass = Higgs.M();
    if(category!=-10){  // really this is nomva cut but -1 means all together here
        if( category>=0 ) {
            fillControlPlots( lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, diphoton_id, -1, isCorrectVertex, evweight,
                    vtx, l, muVtx, mu_ind, elVtx, el_ind, diphobdt_output );
        }
        l.FillHist("all_mass",category+1, Higgs.M(), evweight);
        if( mass>=massMin && mass<=massMax  ) {
            l.FillHist("diphobdt",category+1, diphobdt_output, evweight);
            l.FillHist("process_id",category+1, l.process_id, evweight);
            l.FillHist("mass",category+1, Higgs.M(), evweight);
            l.FillHist("eta",category+1, Higgs.Eta(), evweight);
            l.FillHist("pt",category+1, Higgs.Pt(), evweight);
            if( isCorrectVertex ) { l.FillHist("pt_rv",category+1, Higgs.Pt(), evweight); }
            l.FillHist("nvtx",category+1, l.vtx_std_n, evweight);
            if( isCorrectVertex ) { l.FillHist("nvtx_rv",category+1, l.vtx_std_n, evweight); }

            vtxAna_.setPairID(diphoton_id);
            float vtxProb = vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n);
            l.FillHist2D("probmva_pt",category+1, Higgs.Pt(), l.vtx_std_evt_mva->at(diphoton_id), evweight);
            l.FillHist2D("probmva_nvtx",category+1, l.vtx_std_n, l.vtx_std_evt_mva->at(diphoton_id), evweight);
            if( isCorrectVertex ) {
                l.FillHist2D("probmva_rv_nvtx",category+1, l.vtx_std_n, l.vtx_std_evt_mva->at(diphoton_id), evweight);
            }
            l.FillHist2D("vtxprob_pt",category+1, Higgs.Pt(), vtxProb, evweight);
            l.FillHist2D("vtxprob_nvtx",category+1, l.vtx_std_n, vtxProb, evweight);
            std::vector<int> & vtxlist = l.vtx_std_ranked_list->at(diphoton_id);
            size_t maxv = std::min(vtxlist.size(),(size_t)5);
            for(size_t ivtx=0; ivtx<maxv; ++ivtx) {
                int vtxid = vtxlist.at(ivtx);
                l.FillHist(Form("vtx_mva_%d",ivtx),category+1,vtxAna_.mva(ivtx),evweight);
                if( ivtx > 0 ) {
                    l.FillHist(Form("vtx_dz_%d",ivtx),category+1,
                            vtxAna_.vertexz(ivtx)-vtxAna_.vertexz(l.dipho_vtxind[diphoton_id]),evweight);
                }
            }
            l.FillHist("vtx_nconv",vtxAna_.nconv(0));

            l.FillHist("pho_pt",category+1,lead_p4.Pt(), evweight);
            l.FillHist("pho1_pt",category+1,lead_p4.Pt(), evweight);
            l.FillHist("pho_eta",category+1,lead_p4.Eta(), evweight);
            l.FillHist("pho1_eta",category+1,lead_p4.Eta(), evweight);
            l.FillHist("pho_r9",category+1, lead_r9, evweight);
            l.FillHist("pho1_r9",category+1, lead_r9, evweight);

            l.FillHist("pho_pt",category+1,sublead_p4.Pt(), evweight);
            l.FillHist("pho2_pt",category+1,sublead_p4.Pt(), evweight);
            l.FillHist("pho_eta",category+1,sublead_p4.Eta(), evweight);
            l.FillHist("pho2_eta",category+1,sublead_p4.Eta(), evweight);
            l.FillHist("pho_r9",category+1, sublead_r9, evweight);
            l.FillHist("pho2_r9",category+1, sublead_r9, evweight);

            l.FillHist("pho_n",category+1,l.pho_n, evweight);

            l.FillHist("pho_rawe",category+1,l.sc_raw[l.pho_scind[l.dipho_leadind[diphoton_id]]], evweight);
            l.FillHist("pho_rawe",category+1,l.sc_raw[l.pho_scind[l.dipho_subleadind[diphoton_id]]], evweight);

            TLorentzVector myMet = l.METCorrection2012B(lead_p4, sublead_p4);
            float corrMet    = myMet.Pt();
            float corrMetPhi = myMet.Phi();

            l.FillHist("uncorrmet",     category+1, l.met_pfmet, evweight);
            l.FillHist("uncorrmetPhi",  category+1, l.met_phi_pfmet, evweight);
            l.FillHist("corrmet",       category+1, corrMet,    evweight);
            l.FillHist("corrmetPhi",    category+1, corrMetPhi, evweight);

            if( mvaVbfSelection ) {
                if (!multiclassVbfSelection) {
                    l.FillHist("vbf_mva",category+1,myVBF_MVA,evweight);
                } else {
                    if( vbfVsDiphoVbfSelection ) { 
                        l.FillHist("vbf_mva0",category+1,myVBF_MVA,evweight);
                        l.FillHist("vbf_mva1",category+1,diphobdt_output,evweight);
                    } else { 
                        l.FillHist("vbf_mva0",category+1,myVBF_MVA0,evweight);
                        l.FillHist("vbf_mva1",category+1,myVBF_MVA1,evweight);
                        l.FillHist("vbf_mva2",category+1,myVBF_MVA2,evweight);
                    }
                }

                if (VBFevent){
                    float myweight =  1;
                    float sampleweight = l.sampleContainer[l.current_sample_index].weight();
                    if(evweight*sampleweight!=0) myweight=evweight/sampleweight;
                    l.FillCutPlots(category+1,1,"_sequential",evweight,myweight);
                }
            }
            l.FillHist("rho",category+1,l.rho_algo1,evweight);


            if(category!=-1){
                bool isEBEB  = fabs(lead_p4.Eta() < 1.4442 ) && fabs(sublead_p4.Eta()<1.4442);
                std::string label("final");
                if (VHelevent){
                    ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
                    l.FillHist("ElectronTag_sameVtx",   (int)isEBEB, (float)(elVtx==l.dipho_vtxind[diphoton_id]), evweight);
                    if(cur_type!=0){
                        l.FillHist(Form("ElectronTag_dZtogen_%s",label.c_str()),    (int)isEBEB, (float)((*vtx - *((TVector3*)l.gv_pos->At(0))).Z()), evweight);
                    }

                }

                if (VHmuevent){
                    ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
                    l.FillHist("MuonTag_sameVtx",   (int)isEBEB, (float)(muVtx==l.dipho_vtxind[diphoton_id]), evweight);
                    if(cur_type!=0){
                        l.FillHist(Form("MuonTag_dZtogen_%s",label.c_str()),   (int)isEBEB, (float)((*vtx - *((TVector3*)l.gv_pos->At(0))).Z()), evweight);
                    }
                }

                if (VHmetevent){
                    ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
                    l.FillHist("METTag_sameVtx",   (int)isEBEB, (float)(0==l.dipho_vtxind[diphoton_id]), evweight);
                }

                label.clear();
                label+="nometcut";
                ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);

                label.clear();
                label+="nomvacut";
                if (VHmuevent){
                    ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
                }
                if (VHelevent){
                    ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
                }
                if (VHmetevent){
                    ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
                }

            }
        }
    } else if( mass>=massMin && mass<=massMax  )  { // is -10 = no mva cut
        std::string label("nomvacut");
        if (VHmuevent){
            ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
        }
        if (VHelevent){
            ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
        }
        if (VHmetevent){
            //// ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
        }
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::fillSignalEfficiencyPlots(float weight, LoopAll & l)
{
    //Fill histograms to use as denominator (kinematic pre-selection only) and numerator (selection applied)
    //for single photon ID efficiency calculation.
    int diphoton_id_kinpresel = l.DiphotonMITPreSelection(bdtTrainingType.c_str(),leadEtCut,subleadEtCut,-1.,applyPtoverM, &smeared_pho_energy[0],false,true,-100,-1,false );
    if (diphoton_id_kinpresel>-1) {

        TLorentzVector lead_p4, sublead_p4, Higgs;
        float lead_r9, sublead_r9;
        TVector3 * vtx;
        fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id_kinpresel);

        int ivtx = l.dipho_vtxind[diphoton_id_kinpresel];
        int lead = l.dipho_leadind[diphoton_id_kinpresel];
        int sublead = l.dipho_subleadind[diphoton_id_kinpresel];
        int leadpho_category = l.PhotonCategory(lead, 2, 2);
        int subleadpho_category = l.PhotonCategory(sublead, 2, 2);
        float leadEta = ((TVector3 *)l.sc_xyz->At(l.pho_scind[lead]))->Eta();
        float subleadEta = ((TVector3 *)l.sc_xyz->At(l.pho_scind[sublead]))->Eta();

        float evweight = weight * smeared_pho_weight[lead] * smeared_pho_weight[sublead] * genLevWeight;

        //Fill eta and pt distributions after pre-selection only (efficiency denominator)
        l.FillHist("pho1_pt_presel",0,lead_p4.Pt(), evweight);
        l.FillHist("pho2_pt_presel",0,sublead_p4.Pt(), evweight);
        l.FillHist("pho1_eta_presel",0,leadEta, evweight);
        l.FillHist("pho2_eta_presel",0,subleadEta, evweight);

        l.FillHist("pho1_pt_presel",leadpho_category+1,lead_p4.Pt(), evweight);
        l.FillHist("pho2_pt_presel",subleadpho_category+1,sublead_p4.Pt(), evweight);
        l.FillHist("pho1_eta_presel",leadpho_category+1,leadEta, evweight);
        l.FillHist("pho2_eta_presel",subleadpho_category+1,subleadEta, evweight);

        //Apply single photon CiC selection and fill eta and pt distributions (efficiency numerator)
        std::vector<std::vector<bool> > ph_passcut;
        if( l.PhotonCiCSelectionLevel(lead, ivtx, ph_passcut, 4, 0, &smeared_pho_energy[0]) >=  (LoopAll::phoCiCIDLevel) l.phoSUPERTIGHT) {
            l.FillHist("pho1_pt_sel",0,lead_p4.Pt(), evweight);
            l.FillHist("pho1_eta_sel",0,leadEta, evweight);
            l.FillHist("pho1_pt_sel",leadpho_category+1,lead_p4.Pt(), evweight);
            l.FillHist("pho1_eta_sel",leadpho_category+1,leadEta, evweight);
        }
        if( l.PhotonCiCSelectionLevel(sublead, ivtx, ph_passcut, 4, 1, &smeared_pho_energy[0]) >=  (LoopAll::phoCiCIDLevel) l.phoSUPERTIGHT ) {
            l.FillHist("pho2_pt_sel",0,sublead_p4.Pt(), evweight);
            l.FillHist("pho2_eta_sel",0,subleadEta, evweight);
            l.FillHist("pho2_pt_sel",subleadpho_category+1,sublead_p4.Pt(), evweight);
            l.FillHist("pho2_eta_sel",subleadpho_category+1,subleadEta, evweight);
        }
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::GetBranches(TTree *t, std::set<TBranch *>& s )
{
    vtxAna_.setBranchAdresses(t,"vtx_std_");
    vtxAna_.getBranches(t,"vtx_std_",s);
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::SelectEvents(LoopAll& l, int jentry)
{
    return true;
}
// ----------------------------------------------------------------------------------------------------
double StatAnalysis::GetDifferentialKfactor(double gPT, int Mass)
{
    return 1.0;
}

void StatAnalysis::FillSignalLabelMap(LoopAll & l)
{
    std::map<int,std::pair<TString,double > > & signalMap = l.normalizer()->SignalType();

    for( std::map<int,std::pair<TString,double > >::iterator it=signalMap.begin();
            it!=signalMap.end(); ++it ) {
        signalLabels[it->first] = it->second.first+Form("_mass_m%1.0f", it->second.second);
    }

    signalLabels[-300]="Radion_m300_8TeV";
    signalLabels[-500]="Radion_m500_8TeV";
    signalLabels[-700]="Radion_m700_8TeV";
    signalLabels[-1000]="Radion_m1000_8TeV";
    signalLabels[-1500]="Radion_m1500_8TeV";
    signalLabels[-301]="Radion_m300_8TeV_nm";
    signalLabels[-501]="Radion_m500_8TeV_nm";
    signalLabels[-701]="Radion_m700_8TeV_nm";
    signalLabels[-1001]="Radion_m1000_8TeV_nm";
    signalLabels[-1501]="Radion_m1500_8TeV_nm";
    signalLabels[-302]="Graviton_m300_8TeV";
    signalLabels[-502]="Graviton_m500_8TeV";
    signalLabels[-702]="Graviton_m700_8TeV";
    signalLabels[-1002]="Graviton_m1000_8TeV";
    signalLabels[-1502]="Graviton_m1500_8TeV";
}

std::string StatAnalysis::GetSignalLabel(int id, LoopAll &l){

    // For the lazy man, can return a memeber of the map rather than doing it yourself
    std::map<int,std::string>::iterator it = signalLabels.find(id);
    if( it == signalLabels.end() ) {
	std::string lab = Form("%s_mass_m%1.0f",l.normalizer()->GetProcess(id).Data(),l.normalizer()->GetMass(id));
	it = signalLabels.insert( std::make_pair(id,lab) ).first;
    }
    
    if (it!=signalLabels.end()){
        if(!splitwzh){
            return it->second;
        } else {
            std::string returnstr = it->second;
            if (l.process_id==26){   // wh event
                returnstr.replace(0, 3, "wh");
            } else if (l.process_id==24){   // zh event
                returnstr.replace(0, 3, "zh");
            }
            return returnstr;
        }

    } else {

        std::cerr << "No Signal Type defined in map with id - " << id << std::endl;
        return "NULL";
    }

}

void StatAnalysis::rescaleClusterVariables(LoopAll &l){

    // Data-driven MC scalings
    for (int ipho=0;ipho<l.pho_n;ipho++){

        if (run7TeV4Xanalysis) {

            if( scaleR9Only ) {
                double R9_rescale = (l.pho_isEB[ipho]) ? 1.0048 : 1.00492 ;
                l.pho_r9[ipho]*=R9_rescale;
            } else {
                l.pho_r9[ipho]*=1.0035;
                if (l.pho_isEB[ipho]){ l.pho_sieie[ipho] = (0.87*l.pho_sieie[ipho]) + 0.0011 ;}
                else {l.pho_sieie[ipho]*=0.99;}
                l.sc_seta[l.pho_scind[ipho]]*=0.99;
                l.sc_sphi[l.pho_scind[ipho]]*=0.99;
                energyCorrectedError[ipho] *=(l.pho_isEB[ipho]) ? 1.07 : 1.045 ;
            }

        } else {
            //2012 rescaling from here https://hypernews.cern.ch/HyperNews/CMS/get/higgs2g/752/1/1/2/1/3.html

            if (l.pho_isEB[ipho]) {
                l.pho_r9[ipho] = 1.0045*l.pho_r9[ipho] + 0.0010;
            } else {
                l.pho_r9[ipho] = 1.0086*l.pho_r9[ipho] - 0.0007;
            }
            if( !scaleR9Only ) {
                if (l.pho_isEB[ipho]) {
                    l.pho_s4ratio[ipho] = 1.01894*l.pho_s4ratio[ipho] - 0.01034;
                    l.pho_sieie[ipho] = 0.891832*l.pho_sieie[ipho] + 0.0009133;
                    l.pho_etawidth[ipho] =  1.04302*l.pho_etawidth[ipho] - 0.000618;
                    l.sc_sphi[l.pho_scind[ipho]] =  1.00002*l.sc_sphi[l.pho_scind[ipho]] - 0.000371;
                } else {
                    l.pho_s4ratio[ipho] = 1.04969*l.pho_s4ratio[ipho] - 0.03642;
                    l.pho_sieie[ipho] = 0.99470*l.pho_sieie[ipho] + 0.00003;
                    l.pho_etawidth[ipho] =  0.903254*l.pho_etawidth[ipho] + 0.001346;
                    l.sc_sphi[l.pho_scind[ipho]] =  0.99992*l.sc_sphi[l.pho_scind[ipho]] - 0.00000048;
                    //Agreement not to rescale ES shape (https://hypernews.cern.ch/HyperNews/CMS/get/higgs2g/789/1/1/1/1/1/1/2/1/1.html)
                    //if (l.pho_ESEffSigmaRR[ipho]>0) l.pho_ESEffSigmaRR[ipho] = 1.00023*l.pho_ESEffSigmaRR[ipho] + 0.0913;
                }
            }
        }
        // Scale DYJets sample for now
        /*
           if (l.itype[l.current]==6){
           if (l.pho_isEB[ipho]) {
           energyCorrectedError[ipho] = 1.02693*energyCorrectedError[ipho]-0.0042793;
           } else {
           energyCorrectedError[ipho] = 1.01372*energyCorrectedError[ipho]+0.000156943;
           }
           }
         */
    }
}

void StatAnalysis::ResetAnalysis(){
    // Reset Random Variable on the EnergyResolution Smearer
    if( doEresolSmear ) {
        eResolSmearer->resetRandom();
    }
}

void dumpJet(std::ostream & eventListText, int lab, LoopAll & l, int ijet)
{
    eventListText << std::setprecision(4) << std::scientific
        << "\tjec"      << lab << ":" << l.jet_algoPF1_erescale[ijet]
        << "\tbetaStar" << lab << ":" << l.jet_algoPF1_betaStarClassic[ijet]
        << "\tRMS"      << lab << ":" << l.jet_algoPF1_dR2Mean[ijet]
        ;
}

void dumpPhoton(std::ostream & eventListText, int lab,
        LoopAll & l, int ipho, int ivtx, TLorentzVector & phop4, float * pho_energy_array)
{
    float val_tkisobad = -99;
    for(int iv=0; iv < l.vtx_std_n; iv++) {
        if((*l.pho_pfiso_mycharged04)[ipho][iv] > val_tkisobad) {
            val_tkisobad = (*l.pho_pfiso_mycharged04)[ipho][iv];
        }
    }
    TLorentzVector phop4_badvtx = l.get_pho_p4( ipho, l.pho_tkiso_badvtx_id[ipho], pho_energy_array  );

    float val_tkiso        = (*l.pho_pfiso_mycharged03)[ipho][ivtx];
    float val_ecaliso      = l.pho_pfiso_myphoton03[ipho];
    float val_ecalisobad   = l.pho_pfiso_myphoton04[ipho];
    float val_sieie        = l.pho_sieie[ipho];
    float val_hoe          = l.pho_hoe[ipho];
    float val_r9           = l.pho_r9[ipho];
    float val_conv         = l.pho_isconv[ipho];

    float rhofacbad=0.23, rhofac=0.09;

    float val_isosumoet    = (val_tkiso    + val_ecaliso    - l.rho_algo1 * rhofac )   * 50. / phop4.Et();
    float val_isosumoetbad = (val_tkisobad + val_ecalisobad - l.rho_algo1 * rhofacbad) * 50. / phop4_badvtx.Et();

    // tracker isolation cone energy divided by Et
    float val_trkisooet    = (val_tkiso) * 50. / phop4.Pt();

    eventListText << std::setprecision(4) << std::scientific
        << "\tchIso03"  << lab << ":" << val_tkiso
        << "\tphoIso03" << lab << ":" << val_ecaliso
        << "\tchIso04"  << lab << ":" << val_tkisobad
        << "\tphoIso04" << lab << ":" << val_ecalisobad
        << "\tsIeIe"    << lab << ":" << val_sieie
        << "\thoe"      << lab << ":" << val_hoe
        << "\tecalIso"  << lab << ":" << l.pho_ecalsumetconedr03[ipho]
        << "\thcalIso"  << lab << ":" << l.pho_hcalsumetconedr03[ipho]
        << "\ttrkIso"   << lab << ":" << l.pho_trksumpthollowconedr03[ipho]
        << "\tchIso02"  << lab << ":" << (*l.pho_pfiso_mycharged02)[ipho][ivtx]
        << "\teleVeto"  << lab << ":" << !val_conv
        ;
}


void StatAnalysis::fillOpTree(LoopAll& l, const TLorentzVector & lead_p4, const TLorentzVector & sublead_p4, /*float *smeared_pho_energy,*/ std::vector<float> & smeared_pho_energy, Float_t vtxProb,
        std::pair<int, int> diphoton_index, Int_t diphoton_id, Float_t phoid_mvaout_lead, Float_t phoid_mvaout_sublead,
        Float_t weight, Float_t evweight, Float_t mass, Float_t sigmaMrv, Float_t sigmaMwv,
        const TLorentzVector & Higgs, Float_t diphobdt_output, Int_t category, bool VBFevent, Float_t myVBF_Mjj, Float_t myVBFLeadJPt, 
        Float_t myVBFSubJPt, Int_t nVBFDijetJetCategories, bool isSyst, std::string name1) {

    if(PADEBUG) cerr << "Entering StatAnalysis::fillOpTree" << endl;
    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: lead_p4.Pt()= " << lead_p4.Pt() << "\tsublead_p4.Pt()= " << sublead_p4.Pt() << endl;
// event variables
    l.FillTree("itype", (int)l.itype[l.current]);
	l.FillTree("run",(float)l.run);
	l.FillTree("lumis",(float)l.lumis);
    l.FillTree("event",(float)l.event);
	l.FillTree("weight",(float)weight);
	l.FillTree("evweight",(float)evweight);
    float pu_weight = weight/l.sampleContainer[l.current_sample_index].weight(); // contains also the smearings, not only pu
	l.FillTree("pu_weight",(float)pu_weight);
	l.FillTree("pu_n",(float)l.pu_n);
	l.FillTree("nvtx",(float)l.vtx_std_n);
	l.FillTree("rho", (float)l.rho_algo1);
    l.FillTree("category", (int)category);

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: getting MET corrections" << endl;
    TLorentzVector myMet = l.METCorrection2012B(lead_p4, sublead_p4);

    l.FillTree("met_pfmet", (float)l.met_pfmet);
    l.FillTree("met_phi_pfmet", (float)l.met_phi_pfmet);
    l.FillTree("met_corr_pfmet", (float)myMet.Pt());
    l.FillTree("met_corr_phi_pfmet", (float)myMet.Phi());
    l.FillTree("met_corr_eta_pfmet", (float)myMet.Eta());
    l.FillTree("met_corr_e_pfmet", (float)myMet.Energy());

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling photon information" << endl;
// photon variables
	l.FillTree("ph1_e",(float)lead_p4.E());
	l.FillTree("ph2_e",(float)sublead_p4.E());
	l.FillTree("ph1_pt",(float)lead_p4.Pt());
	l.FillTree("ph2_pt",(float)sublead_p4.Pt());
	l.FillTree("ph1_phi",(float)lead_p4.Phi());
	l.FillTree("ph2_phi",(float)sublead_p4.Phi());
	l.FillTree("ph1_eta",(float)lead_p4.Eta());
	l.FillTree("ph2_eta",(float)sublead_p4.Eta());
	l.FillTree("ph1_r9",(float)l.pho_r9[diphoton_index.first]);
	l.FillTree("ph2_r9",(float)l.pho_r9[diphoton_index.second]);
//	l.FillTree("ph1_isPrompt", (int)l.GenParticleInfo(diphoton_index.first, l.dipho_vtxind[diphoton_id], 0.1));
//	l.FillTree("ph2_isPrompt", (int)l.GenParticleInfo(diphoton_index.second, l.dipho_vtxind[diphoton_id], 0.1));
    l.FillTree("ph1_isPrompt", (int)l.pho_genmatched[diphoton_index.first]); // Alternative definition for gen photon matching: Nicolas' definition need re-reduction
    l.FillTree("ph2_isPrompt", (int)l.pho_genmatched[diphoton_index.second]);
	l.FillTree("ph1_SCEta", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.first]))->Eta());
	l.FillTree("ph2_SCEta", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.second]))->Eta());
    l.FillTree("ph1_SCPhi", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.first]))->Phi());
    l.FillTree("ph2_SCPhi", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.second]))->Phi());
	l.FillTree("ph1_hoe", (float)l.pho_hoe[diphoton_index.first]);
	l.FillTree("ph2_hoe", (float)l.pho_hoe[diphoton_index.second]);
	l.FillTree("ph1_sieie", (float)l.pho_sieie[diphoton_index.first]);
	l.FillTree("ph2_sieie", (float)l.pho_sieie[diphoton_index.second]);
	l.FillTree("ph1_pfchargedisogood03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.first][l.dipho_vtxind[diphoton_id]]);
	l.FillTree("ph2_pfchargedisogood03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.second][l.dipho_vtxind[diphoton_id]]);
	double pho1_pfchargedisobad04 = 0.;
	double pho2_pfchargedisobad04 = 0.;
	double pho1_pfchargedisobad03 = 0.;
	double pho2_pfchargedisobad03 = 0.;
	int pho1_ivtxpfch04bad=-1;

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: computing and storing isolations wrt chosen vertex" << endl;
	for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++){
        if ((*l.pho_pfiso_mycharged04)[diphoton_index.first][ivtx]>pho1_pfchargedisobad04){
            pho1_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.first][ivtx]>pho1_pfchargedisobad04;
	        pho1_ivtxpfch04bad = ivtx;
    }
	}
	pho1_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.first][pho1_ivtxpfch04bad];
	l.FillTree("ph1_pfchargedisobad04", (float)pho1_pfchargedisobad04);
   int pho2_ivtxpfch04bad=-1;
    for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++){
      if ((*l.pho_pfiso_mycharged04)[diphoton_index.second][ivtx]>pho2_pfchargedisobad04){
        pho2_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.second][ivtx]>pho2_pfchargedisobad04;
          pho2_ivtxpfch04bad = ivtx;
      }
    }
    pho2_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.second][pho2_ivtxpfch04bad];
	l.FillTree("ph2_pfchargedisobad04", (float)pho2_pfchargedisobad04);
	l.FillTree("ph1_etawidth", (float)l.pho_etawidth[diphoton_index.first]);
	l.FillTree("ph2_etawidth", (float)l.pho_etawidth[diphoton_index.second]);
	l.FillTree("ph1_phiwidth", (float)(l.sc_sphi[l.pho_scind[diphoton_index.first]]));
	l.FillTree("ph2_phiwidth", (float)(l.sc_sphi[l.pho_scind[diphoton_index.second]]));
	l.FillTree("ph1_eseffssqrt", (float)sqrt(l.pho_eseffsixix[diphoton_index.first]*l.pho_eseffsixix[diphoton_index.first]+l.pho_eseffsiyiy[diphoton_index.first]*l.pho_eseffsiyiy[diphoton_index.first]));
	l.FillTree("ph2_eseffssqrt", (float)sqrt(l.pho_eseffsixix[diphoton_index.second]*l.pho_eseffsixix[diphoton_index.second]+l.pho_eseffsiyiy[diphoton_index.second]*l.pho_eseffsiyiy[diphoton_index.second]));
	l.FillTree("ph1_pfchargedisobad03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.first][pho1_ivtxpfch04bad]);
	l.FillTree("ph2_pfchargedisobad03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.second][pho2_ivtxpfch04bad]);
	l.FillTree("ph1_sieip", (float)l.pho_sieip[diphoton_index.first]);
	l.FillTree("ph2_sieip", (float)l.pho_sieip[diphoton_index.second]);
	l.FillTree("ph1_sipip", (float)l.pho_sipip[diphoton_index.first]);
	l.FillTree("ph2_sipip", (float)l.pho_sipip[diphoton_index.second]);
	l.FillTree("ph1_ecaliso", (float)l.pho_pfiso_myphoton03[diphoton_index.first]);
	l.FillTree("ph2_ecaliso", (float)l.pho_pfiso_myphoton03[diphoton_index.second]);
	l.FillTree("ph1_ecalisobad", (float)l.pho_pfiso_myphoton04[diphoton_index.first]);
	l.FillTree("ph2_ecalisobad", (float)l.pho_pfiso_myphoton04[diphoton_index.second]);
    TLorentzVector ph1_badvtx = l.get_pho_p4( diphoton_index.first, l.pho_tkiso_badvtx_id[diphoton_index.first], &smeared_pho_energy[0] );
	l.FillTree("ph1_badvtx_Et", (float)ph1_badvtx.Et());
    TLorentzVector ph2_badvtx = l.get_pho_p4( diphoton_index.second, l.pho_tkiso_badvtx_id[diphoton_index.second], &smeared_pho_energy[0] );
	l.FillTree("ph2_badvtx_Et", (float)ph2_badvtx.Et());
	l.FillTree("ph1_isconv", (float)l.pho_isconv[diphoton_index.first]);
	l.FillTree("ph2_isconv", (float)l.pho_isconv[diphoton_index.second]);
    vector<vector<bool> > ph_passcut;
    int ph1_ciclevel = l.PhotonCiCPFSelectionLevel(diphoton_index.first, l.dipho_vtxind[diphoton_id], ph_passcut, 4, 0, &smeared_pho_energy[0]);
    int ph2_ciclevel = l.PhotonCiCPFSelectionLevel(diphoton_index.second, l.dipho_vtxind[diphoton_id], ph_passcut, 4, 0, &smeared_pho_energy[0]);
    l.FillTree("ph1_ciclevel", (int)ph1_ciclevel);
    l.FillTree("ph2_ciclevel", (int)ph2_ciclevel);
    l.FillTree("ph1_sigmaEoE", (float)l.pho_regr_energyerr[diphoton_index.first]/(float)l.pho_regr_energy[diphoton_index.first]);
    l.FillTree("ph2_sigmaEoE", (float)l.pho_regr_energyerr[diphoton_index.second]/(float)l.pho_regr_energy[diphoton_index.second]);
	l.FillTree("ph1_ptoM", (float)lead_p4.Pt()/mass);
	l.FillTree("ph2_ptoM", (float)sublead_p4.Pt()/mass);
	l.FillTree("ph1_isEB", (int)l.pho_isEB[diphoton_index.first]);
	l.FillTree("ph2_isEB", (int)l.pho_isEB[diphoton_index.second]);
    float s4ratio1 = l.pho_e2x2[diphoton_index.first]/l.pho_e5x5[diphoton_index.first];
    float s4ratio2 = l.pho_e2x2[diphoton_index.second]/l.pho_e5x5[diphoton_index.second];
    l.FillTree("ph1_s4ratio", s4ratio1);
    l.FillTree("ph2_s4ratio", s4ratio2);
    l.FillTree("ph1_e3x3", l.pho_e3x3[diphoton_index.first]);
    l.FillTree("ph2_e3x3", l.pho_e3x3[diphoton_index.second]);
    l.FillTree("ph1_e5x5", l.pho_e5x5[diphoton_index.first]);
    l.FillTree("ph2_e5x5", l.pho_e5x5[diphoton_index.second]);

// diphoton variables
	l.FillTree("PhotonsMass",(float)mass);
    TLorentzVector diphoton = lead_p4 + sublead_p4;
	l.FillTree("dipho_E", (float)diphoton.Energy());
	l.FillTree("dipho_pt", (float)diphoton.Pt());
	l.FillTree("dipho_eta", (float)diphoton.Eta());
	l.FillTree("dipho_phi", (float)diphoton.Phi());
	l.FillTree("dipho_cosThetaStar_CS", (float)getCosThetaCS(lead_p4, sublead_p4, l.sqrtS));
    float dipho_tanhYStar = tanh(
        (float)(fabs(lead_p4.Rapidity() - sublead_p4.Rapidity()))/(float)(2.0)
    );
	l.FillTree("dipho_tanhYStar", (float)dipho_tanhYStar);
	l.FillTree("dipho_Y", (float)diphoton.Rapidity());
    TVector3* vtx = (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]);

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: vertex variables" << endl;
// vertices variables
    l.FillTree("vtx_ind", (int)l.dipho_vtxind[diphoton_id]);
	l.FillTree("vtx_x", (float)vtx->X());
	l.FillTree("vtx_y", (float)vtx->Y());
	l.FillTree("vtx_z", (float)vtx->Z());
    vtxAna_.setPairID(diphoton_id);
    l.FillTree("vtx_mva", (float)vtxAna_.mva(0));
    l.FillTree("vtx_mva_2", (float)vtxAna_.mva(1));
    l.FillTree("vtx_mva_3", (float)vtxAna_.mva(2));
    l.FillTree("vtx_ptbal", (float)vtxAna_.ptbal(0));
    l.FillTree("vtx_ptasym", (float)vtxAna_.ptasym(0));
    l.FillTree("vtx_logsumpt2", (float)vtxAna_.logsumpt2(0));
    l.FillTree("vtx_pulltoconv", (float)vtxAna_.pulltoconv(0));
    l.FillTree("vtx_prob", (float)vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n));

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: select jets" << endl;
// jet variables
    vector<int> jets;
    jets = l.SelectJets(lead_p4, sublead_p4);

    if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling jet info" << endl;
    l.FillTree("njets_passing_kLooseID",(int)jets.size());
    int njets_passing_kLooseID_and_CSVL = 0;
    int njets_passing_kLooseID_and_CSVM = 0;
    int njets_passing_kLooseID_and_CSVT = 0;
    for(int ijet=0 ; ijet < jets.size() ; ijet++)
    {
        if( PADEBUG ) cout << "l.jet_algoPF1_emfrac[jets[" << ijet << "]]= " << l.jet_algoPF1_emfrac[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_hadfrac[jets[" << ijet << "]]= " << l.jet_algoPF1_hadfrac[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_ntk[jets[" << ijet << "]]= " << l.jet_algoPF1_ntk[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_nNeutrals[jets[" << ijet << "]]= " << (int)l.jet_algoPF1_nNeutrals[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_nCharged[jets[" << ijet << "]]= " << l.jet_algoPF1_nCharged[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_genPt[jets[" << ijet << "]]= " << l.jet_algoPF1_genPt[jets[ijet]] << endl;
        if( PADEBUG ) cout << "l.jet_algoPF1_csvBtag[jets[" << ijet << "]]= " << l.jet_algoPF1_csvBtag[jets[ijet]] << endl;
        float csv = l.jet_algoPF1_csvBtag[jets[ijet]];
        if( csv > 0.244 )
            njets_passing_kLooseID_and_CSVL++;
        if( csv > 0.679 )
            njets_passing_kLooseID_and_CSVM++;
        if( csv > 0.898 )
            njets_passing_kLooseID_and_CSVT++;
    }
    l.FillTree("njets_passing_kLooseID_and_CSVL", (int)njets_passing_kLooseID_and_CSVL); 
    l.FillTree("njets_passing_kLooseID_and_CSVM", (int)njets_passing_kLooseID_and_CSVM); 
    l.FillTree("njets_passing_kLooseID_and_CSVT", (int)njets_passing_kLooseID_and_CSVT); 

    if( PADEBUG ) cout << "jets.size()= " << jets.size() << endl;
    if(jets.size()>0){
        if(PADEBUG) cout << "processing jet 0" << endl;
        if(PADEBUG) cout << "4-momentum of the jet" << endl;
        TLorentzVector* jet1 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]);
	    l.FillTree("j1_e",(float)jet1->Energy());
    	l.FillTree("j1_pt",(float)jet1->Pt());
	    l.FillTree("j1_phi",(float)jet1->Phi());
    	l.FillTree("j1_eta",(float)jet1->Eta());
        if(PADEBUG) cout << "jet1->Energy()= " << jet1->Energy() << "\tjet1->Pt()= " << jet1->Pt() << "\tjet1->Phi()= " << jet1->Phi() << "\tjet1->Eta()= " << jet1->Eta() << "\tjets[0]= " << jets[0] << endl;
        if(PADEBUG) cout << "trying accesses: " << endl;
        TLorentzVector j1_jecD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]), jets[0], 1, -1., 0,  0.);
        TLorentzVector j1_jecU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]), jets[0], 1, 1., 0,  0.);
        TLorentzVector j1_jerD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]), jets[0], 0,  0., 1, -1.);
        TLorentzVector j1_jerC = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]), jets[0], 0,  0., 1,  0.);
        TLorentzVector j1_jerU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]), jets[0], 0,  0., 1, 1.);
        if(PADEBUG) cout << "Now that we have JEC and JER, store it" << endl;
	    l.FillTree("j1_jecD_e",(float)j1_jecD.Energy());
    	l.FillTree("j1_jecD_pt",(float)j1_jecD.Pt());
	    l.FillTree("j1_jecD_phi",(float)j1_jecD.Phi());
    	l.FillTree("j1_jecD_eta",(float)j1_jecD.Eta());
	    l.FillTree("j1_jecU_e",(float)j1_jecU.Energy());
    	l.FillTree("j1_jecU_pt",(float)j1_jecU.Pt());
	    l.FillTree("j1_jecU_phi",(float)j1_jecU.Phi());
    	l.FillTree("j1_jecU_eta",(float)j1_jecU.Eta());
	    l.FillTree("j1_jerD_e",(float)j1_jerD.Energy());
    	l.FillTree("j1_jerD_pt",(float)j1_jerD.Pt());
	    l.FillTree("j1_jerD_phi",(float)j1_jerD.Phi());
    	l.FillTree("j1_jerD_eta",(float)j1_jerD.Eta());
	    l.FillTree("j1_jerC_e",(float)j1_jerC.Energy());
    	l.FillTree("j1_jerC_pt",(float)j1_jerC.Pt());
	    l.FillTree("j1_jerC_phi",(float)j1_jerC.Phi());
    	l.FillTree("j1_jerC_eta",(float)j1_jerC.Eta());
	    l.FillTree("j1_jerU_e",(float)j1_jerU.Energy());
    	l.FillTree("j1_jerU_pt",(float)j1_jerU.Pt());
	    l.FillTree("j1_jerU_phi",(float)j1_jerU.Phi());
    	l.FillTree("j1_jerU_eta",(float)j1_jerU.Eta());
	    //l.FillTree("j1_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[0]]);
    	l.FillTree("j1_beta", (float)l.jet_algoPF1_beta[jets[0]]);
	    l.FillTree("j1_betaStar", (float)l.jet_algoPF1_betaStar[jets[0]]);
    	l.FillTree("j1_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[0]]);
	    l.FillTree("j1_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[0]]);
        l.FillTree("j1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[0]]);
        l.FillTree("j1_csvMvaBtag", (float)l.jet_algoPF1_csvMvaBtag[jets[0]]);
        l.FillTree("j1_jetProbBtag", (float)l.jet_algoPF1_jetProbBtag[jets[0]]);
        l.FillTree("j1_tcheBtag", (float)l.jet_algoPF1_tcheBtag[jets[0]]);
        //BtagSF variables (Badder)
        if(l.itype[l.current] == -301 || l.itype[l.current] == -501 || l.itype[l.current] == -701 || l.itype[l.current] == -1001 || l.itype[l.current] == -1501){
           if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling BtagSF variables, jet1" << endl;
           int flavour = jetFlavReader->getJetFlavour((int)l.lumis, (int)l.event,jet1); 
           float btagSF = SFReader->getSF(jet1,flavour,l.jet_algoPF1_csvBtag[jets[0]]);
           float btagSFErrorUp = SFReader->getSFErrorUp(jet1,flavour,l.jet_algoPF1_csvBtag[jets[0]]);
           float btagSFErrorDown = SFReader->getSFErrorDown(jet1,flavour,l.jet_algoPF1_csvBtag[jets[0]]);
           float btagEff = EffReader->getBtagEfficiency(jet1,l.jet_algoPF1_csvBtag[jets[0]],flavour);
           float btagEffError = EffReader->getBtagEfficiencyError(jet1,l.jet_algoPF1_csvBtag[jets[0]],flavour);
           l.FillTree("j1_flavour",(int)flavour);
           l.FillTree("j1_btagSF",(float)btagSF);
           l.FillTree("j1_btagSFErrorUp",(float)btagSFErrorUp);
           l.FillTree("j1_btagSFErrorDown",(float)btagSFErrorDown);
           l.FillTree("j1_btagEff",(float)btagEff);
           l.FillTree("j1_btagEffError",(float)btagEffError);
        }else{
           l.FillTree("j1_flavour",(int)0);
           l.FillTree("j1_btagSF",(float)-1001.);
           l.FillTree("j1_btagSFErrorUp",(float)-1001.);
           l.FillTree("j1_btagSFErrorDown",(float)-1001.);
           l.FillTree("j1_btagEff",(float)-1001.);
           l.FillTree("j1_btagEffError",(float)-1001.);
        }
        l.FillTree("j1_bgenMatched", (float)l.jet_algoPF1_bgenMatched[jets[0]]);
		l.FillTree("j1_nSecondaryVertices", (float)l.jet_algoPF1_nSecondaryVertices[jets[0]]);
		l.FillTree("j1_secVtxPt", (float)l.jet_algoPF1_secVtxPt[jets[0]]);
		l.FillTree("j1_secVtx3dL", (float)l.jet_algoPF1_secVtx3dL[jets[0]]);
		l.FillTree("j1_secVtx3deL", (float)l.jet_algoPF1_secVtx3deL[jets[0]]);
		l.FillTree("j1_emfrac", (float)l.jet_algoPF1_emfrac[jets[0]]);
		l.FillTree("j1_hadfrac", (float)l.jet_algoPF1_hadfrac[jets[0]]);
		l.FillTree("j1_ntk", (int)l.jet_algoPF1_ntk[jets[0]]);
		l.FillTree("j1_nNeutrals", (int)l.jet_algoPF1_nNeutrals[jets[0]]);
		l.FillTree("j1_nCharged", (int)l.jet_algoPF1_nCharged[jets[0]]);
		l.FillTree("j1_genPt", (float)l.jet_algoPF1_genPt[jets[0]]);
    } else {
	    l.FillTree("j1_e",(float)-1001.);
    	l.FillTree("j1_pt",(float)-1001.);
	    l.FillTree("j1_phi",(float)-1001.);
    	l.FillTree("j1_eta",(float)-1001.);
	    l.FillTree("j1_jecD_e",(float)-1001.);
    	l.FillTree("j1_jecD_pt",(float)-1001.);
	    l.FillTree("j1_jecD_phi",(float)-1001.);
    	l.FillTree("j1_jecD_eta",(float)-1001.);
	    l.FillTree("j1_jecU_e",(float)-1001.);
    	l.FillTree("j1_jecU_pt",(float)-1001.);
	    l.FillTree("j1_jecU_phi",(float)-1001.);
    	l.FillTree("j1_jecU_eta",(float)-1001.);
	    l.FillTree("j1_jerD_e",(float)-1001.);
    	l.FillTree("j1_jerD_pt",(float)-1001.);
	    l.FillTree("j1_jerD_phi",(float)-1001.);
    	l.FillTree("j1_jerD_eta",(float)-1001.);
	    l.FillTree("j1_jerC_e",(float)-1001.);
    	l.FillTree("j1_jerC_pt",(float)-1001.);
	    l.FillTree("j1_jerC_phi",(float)-1001.);
    	l.FillTree("j1_jerC_eta",(float)-1001.);
	    l.FillTree("j1_jerU_e",(float)-1001.);
    	l.FillTree("j1_jerU_pt",(float)-1001.);
	    l.FillTree("j1_jerU_phi",(float)-1001.);
    	l.FillTree("j1_jerU_eta",(float)-1001.);
	    //l.FillTree("j1_cutbased_wp_level", (float)-1001.);
    	l.FillTree("j1_beta", (float)-1001.);
	    l.FillTree("j1_betaStar", (float)-1001.);
    	l.FillTree("j1_betaStarClassic", (float)-1001.);
	    l.FillTree("j1_dR2Mean", (float)-1001.);
        l.FillTree("j1_csvBtag", (float)-1001.);
        l.FillTree("j1_csvMvaBtag", (float)-1001.);
        l.FillTree("j1_jetProbBtag", (float)-1001.);
        l.FillTree("j1_tcheBtag", (float)-1001.);
        l.FillTree("j1_flavour",(int)0);
        l.FillTree("j1_btagSF",(float)-1001.);
        l.FillTree("j1_btagSFErrorUp",(float)-1001.);
        l.FillTree("j1_btagSFErrorDown",(float)-1001.);
        l.FillTree("j1_btagEff",(float)-1001.);
        l.FillTree("j1_btagEffError",(float)-1001.);
        l.FillTree("j1_bgenMatched", (float)-1001.);
		l.FillTree("j1_nSecondaryVertices", (float)-1001.);
		l.FillTree("j1_secVtxPt", (float)-1001.);
		l.FillTree("j1_secVtx3dL", (float)-1001.);
		l.FillTree("j1_secVtx3deL", (float)-1001.);
		l.FillTree("j1_emfrac", (float)-1001.);
		l.FillTree("j1_hadfrac", (float)-1001.);
		l.FillTree("j1_ntk", (int)-1001);
		l.FillTree("j1_nNeutrals", (int)-1001);
		l.FillTree("j1_nCharged", (int)-1001);
		l.FillTree("j1_genPt", (float)-1001);
    } // end if njets > 0

    if(jets.size()>1){
        TLorentzVector* jet2 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]);
	    l.FillTree("j2_e",(float)jet2->Energy());
	    l.FillTree("j2_pt",(float)jet2->Pt());
	    l.FillTree("j2_phi",(float)jet2->Phi());
	    l.FillTree("j2_eta",(float)jet2->Eta());
        TLorentzVector j2_jecD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]), jets[1], 1, -1., 0,  0.);
        TLorentzVector j2_jecU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]), jets[1], 1, +1., 0,  0.);
        TLorentzVector j2_jerD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]), jets[1], 0,  0., 1, -1.);
        TLorentzVector j2_jerC = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]), jets[1], 0,  0., 1,  0.);
        TLorentzVector j2_jerU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]), jets[1], 0,  0., 1, +1.);
	    l.FillTree("j2_jecD_e",(float)j2_jecD.Energy());
    	l.FillTree("j2_jecD_pt",(float)j2_jecD.Pt());
	    l.FillTree("j2_jecD_phi",(float)j2_jecD.Phi());
    	l.FillTree("j2_jecD_eta",(float)j2_jecD.Eta());
	    l.FillTree("j2_jecU_e",(float)j2_jecU.Energy());
    	l.FillTree("j2_jecU_pt",(float)j2_jecU.Pt());
	    l.FillTree("j2_jecU_phi",(float)j2_jecU.Phi());
    	l.FillTree("j2_jecU_eta",(float)j2_jecU.Eta());
	    l.FillTree("j2_jerD_e",(float)j2_jerD.Energy());
    	l.FillTree("j2_jerD_pt",(float)j2_jerD.Pt());
	    l.FillTree("j2_jerD_phi",(float)j2_jerD.Phi());
    	l.FillTree("j2_jerD_eta",(float)j2_jerD.Eta());
	    l.FillTree("j2_jerC_e",(float)j2_jerC.Energy());
    	l.FillTree("j2_jerC_pt",(float)j2_jerC.Pt());
	    l.FillTree("j2_jerC_phi",(float)j2_jerC.Phi());
    	l.FillTree("j2_jerC_eta",(float)j2_jerC.Eta());
	    l.FillTree("j2_jerU_e",(float)j2_jerU.Energy());
    	l.FillTree("j2_jerU_pt",(float)j2_jerU.Pt());
	    l.FillTree("j2_jerU_phi",(float)j2_jerU.Phi());
    	l.FillTree("j2_jerU_eta",(float)j2_jerU.Eta());
	    //l.FillTree("j2_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[1]]);
    	l.FillTree("j2_beta", (float)l.jet_algoPF1_beta[jets[1]]);
    	l.FillTree("j2_betaStar", (float)l.jet_algoPF1_betaStar[jets[1]]);
    	l.FillTree("j2_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[1]]);
    	l.FillTree("j2_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[1]]);
        l.FillTree("j2_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[1]]);
        l.FillTree("j2_csvMvaBtag", (float)l.jet_algoPF1_csvMvaBtag[jets[1]]);
        l.FillTree("j2_jetProbBtag", (float)l.jet_algoPF1_jetProbBtag[jets[1]]);
        l.FillTree("j2_tcheBtag", (float)l.jet_algoPF1_tcheBtag[jets[1]]);
        //BtagSF variables (Badder)
        if(l.itype[l.current] == -301 || l.itype[l.current] == -501 || l.itype[l.current] == -701 || l.itype[l.current] == -1001 || l.itype[l.current] == -1501){
           if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling BtagSF variables, jet2" << endl;
           int flavour = jetFlavReader->getJetFlavour((int)l.lumis, (int)l.event,jet2); 
           float btagSF = SFReader->getSF(jet2,flavour,l.jet_algoPF1_csvBtag[jets[1]]);
           float btagSFErrorUp = SFReader->getSFErrorUp(jet2,flavour,l.jet_algoPF1_csvBtag[jets[1]]);
           float btagSFErrorDown = SFReader->getSFErrorDown(jet2,flavour,l.jet_algoPF1_csvBtag[jets[1]]);
           float btagEff = EffReader->getBtagEfficiency(jet2,l.jet_algoPF1_csvBtag[jets[1]],flavour);
           float btagEffError = EffReader->getBtagEfficiencyError(jet2,l.jet_algoPF1_csvBtag[jets[1]],flavour);
           l.FillTree("j2_flavour",(int)flavour);
           l.FillTree("j2_btagSF",(float)btagSF);
           l.FillTree("j2_btagSFErrorUp",(float)btagSFErrorUp);
           l.FillTree("j2_btagSFErrorDown",(float)btagSFErrorDown);
           l.FillTree("j2_btagEff",(float)btagEff);
           l.FillTree("j2_btagEffError",(float)btagEffError);
        }else{
           l.FillTree("j2_flavour",(int)0);
           l.FillTree("j2_btagSF",(float)-1001.);
           l.FillTree("j2_btagSFErrorUp",(float)-1001.);
           l.FillTree("j2_btagSFErrorDown",(float)-1001.);
           l.FillTree("j2_btagEff",(float)-1001.);
           l.FillTree("j2_btagEffError",(float)-1001.);
        }
        l.FillTree("j2_bgenMatched", (float)l.jet_algoPF1_bgenMatched[jets[1]]);
		l.FillTree("j2_nSecondaryVertices", (float)l.jet_algoPF1_nSecondaryVertices[jets[1]]);
		l.FillTree("j2_secVtxPt", (float)l.jet_algoPF1_secVtxPt[jets[1]]);
		l.FillTree("j2_secVtx3dL", (float)l.jet_algoPF1_secVtx3dL[jets[1]]);
		l.FillTree("j2_secVtx3deL", (float)l.jet_algoPF1_secVtx3deL[jets[1]]);
		l.FillTree("j2_emfrac", (float)l.jet_algoPF1_emfrac[jets[1]]);
		l.FillTree("j2_hadfrac", (float)l.jet_algoPF1_hadfrac[jets[1]]);
		l.FillTree("j2_ntk", (int)l.jet_algoPF1_ntk[jets[1]]);
		l.FillTree("j2_nNeutrals", (int)l.jet_algoPF1_nNeutrals[jets[1]]);
		l.FillTree("j2_nCharged", (int)l.jet_algoPF1_nCharged[jets[1]]);
		l.FillTree("j2_genPt", (float)l.jet_algoPF1_genPt[jets[1]]);
    } else {
	    l.FillTree("j2_e",(float)-1001.);
	    l.FillTree("j2_pt",(float)-1001.);
	    l.FillTree("j2_phi",(float)-1001.);
	    l.FillTree("j2_eta",(float)-1001.);
	    l.FillTree("j2_jecD_e",(float)-1001.);
    	l.FillTree("j2_jecD_pt",(float)-1001.);
	    l.FillTree("j2_jecD_phi",(float)-1001.);
    	l.FillTree("j2_jecD_eta",(float)-1001.);
	    l.FillTree("j2_jecU_e",(float)-1001.);
    	l.FillTree("j2_jecU_pt",(float)-1001.);
	    l.FillTree("j2_jecU_phi",(float)-1001.);
    	l.FillTree("j2_jecU_eta",(float)-1001.);
	    l.FillTree("j2_jerD_e",(float)-1001.);
    	l.FillTree("j2_jerD_pt",(float)-1001.);
	    l.FillTree("j2_jerD_phi",(float)-1001.);
    	l.FillTree("j2_jerD_eta",(float)-1001.);
	    l.FillTree("j2_jerC_e",(float)-1001.);
    	l.FillTree("j2_jerC_pt",(float)-1001.);
	    l.FillTree("j2_jerC_phi",(float)-1001.);
    	l.FillTree("j2_jerC_eta",(float)-1001.);
	    l.FillTree("j2_jerU_e",(float)-1001.);
    	l.FillTree("j2_jerU_pt",(float)-1001.);
	    l.FillTree("j2_jerU_phi",(float)-1001.);
    	l.FillTree("j2_jerU_eta",(float)-1001.);
	    //l.FillTree("j2_cutbased_wp_level", (float)-1001.);
    	l.FillTree("j2_beta", (float)-1001.);
    	l.FillTree("j2_betaStar", (float)-1001.);
    	l.FillTree("j2_betaStarClassic", (float)-1001.);
    	l.FillTree("j2_dR2Mean", (float)-1001.);
        l.FillTree("j2_csvBtag", (float)-1001.);
        l.FillTree("j2_csvMvaBtag", (float)-1001.);
        l.FillTree("j2_jetProbBtag", (float)-1001.);
        l.FillTree("j2_tcheBtag", (float)-1001.);
        l.FillTree("j2_flavour",(int)0);
        l.FillTree("j2_btagSF",(float)-1001.);
        l.FillTree("j2_btagSFErrorUp",(float)-1001.);
        l.FillTree("j2_btagSFErrorDown",(float)-1001.);
        l.FillTree("j2_btagEff",(float)-1001.);
        l.FillTree("j2_btagEffError",(float)-1001.);
        l.FillTree("j2_bgenMatched", (float)-1001.);
		l.FillTree("j2_nSecondaryVertices", (float)-1001.);
		l.FillTree("j2_secVtxPt", (float)-1001.);
		l.FillTree("j2_secVtx3dL", (float)-1001.);
		l.FillTree("j2_secVtx3deL", (float)-1001.);
		l.FillTree("j2_emfrac", (float)-1001.);
		l.FillTree("j2_hadfrac", (float)-1001.);
		l.FillTree("j2_ntk", (int)-1001);
		l.FillTree("j2_nNeutrals", (int)-1001);
		l.FillTree("j2_nCharged", (int)-1001);
		l.FillTree("j2_genPt", (float)-1001);
    } // end if njets > 1

    if(jets.size()>1){
        if(PADEBUG) cout << "processing jet 1" << endl;
        TLorentzVector* jet1 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]);
        TLorentzVector* jet2 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]);
        // dijet variables
        TLorentzVector dijet = *jet1 + *jet2;
	    l.FillTree("JetsMass", (float)dijet.M());
        l.FillTree("dijet_E", (float)dijet.Energy());
        l.FillTree("dijet_Pt", (float)dijet.Pt());
        l.FillTree("dijet_Eta", (float)dijet.Eta());
        l.FillTree("dijet_Phi", (float)dijet.Phi());

        // radion variables
        TLorentzVector radion = dijet + diphoton;
	    l.FillTree("RadMass",(float)radion.M());
        l.FillTree("radion_E", (float)radion.Energy());
        l.FillTree("radion_Pt", (float)radion.Pt());
        l.FillTree("radion_Eta", (float)radion.Eta());
        l.FillTree("radion_Phi", (float)radion.Phi());
    } else {
	    l.FillTree("JetsMass", (float)-1001.);
        l.FillTree("dijet_E", (float)-1001.);
        l.FillTree("dijet_Pt", (float)-1001.);
        l.FillTree("dijet_Eta", (float)-1001.);
        l.FillTree("dijet_Phi", (float)-1001.);
	    l.FillTree("RadMass",(float)-1001.);
        l.FillTree("radion_E", (float)-1001.);
        l.FillTree("radion_Pt", (float)-1001.);
        l.FillTree("radion_Eta", (float)-1001.);
        l.FillTree("radion_Phi", (float)-1001.);
    } // if 2 jets



    TLorentzVector* jet3 = new TLorentzVector();
    TLorentzVector* jet4 = new TLorentzVector();



    if(jets.size() > 2){
        if(PADEBUG) cout << "processing jet 2" << endl;
        jet3 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]);
    	l.FillTree("j3_e",(float)jet3->Energy());
	    l.FillTree("j3_pt",(float)jet3->Pt());
    	l.FillTree("j3_phi",(float)jet3->Phi());
    	l.FillTree("j3_eta",(float)jet3->Eta());
        TLorentzVector j3_jecD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]), jets[2], 1, -1., 0,  0.);
        TLorentzVector j3_jecU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]), jets[2], 1, +1., 0,  0.);
        TLorentzVector j3_jerD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]), jets[2], 0,  0., 1, -1.);
        TLorentzVector j3_jerC = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]), jets[2], 0,  0., 1,  0.);
        TLorentzVector j3_jerU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]), jets[2], 0,  0., 1, +1.);
	    l.FillTree("j3_jecD_e",(float)j3_jecD.Energy());
    	l.FillTree("j3_jecD_pt",(float)j3_jecD.Pt());
	    l.FillTree("j3_jecD_phi",(float)j3_jecD.Phi());
    	l.FillTree("j3_jecD_eta",(float)j3_jecD.Eta());
	    l.FillTree("j3_jecU_e",(float)j3_jecU.Energy());
    	l.FillTree("j3_jecU_pt",(float)j3_jecU.Pt());
	    l.FillTree("j3_jecU_phi",(float)j3_jecU.Phi());
    	l.FillTree("j3_jecU_eta",(float)j3_jecU.Eta());
	    l.FillTree("j3_jerD_e",(float)j3_jerD.Energy());
    	l.FillTree("j3_jerD_pt",(float)j3_jerD.Pt());
	    l.FillTree("j3_jerD_phi",(float)j3_jerD.Phi());
    	l.FillTree("j3_jerD_eta",(float)j3_jerD.Eta());
	    l.FillTree("j3_jerC_e",(float)j3_jerC.Energy());
    	l.FillTree("j3_jerC_pt",(float)j3_jerC.Pt());
	    l.FillTree("j3_jerC_phi",(float)j3_jerC.Phi());
    	l.FillTree("j3_jerC_eta",(float)j3_jerC.Eta());
	    l.FillTree("j3_jerU_e",(float)j3_jerU.Energy());
    	l.FillTree("j3_jerU_pt",(float)j3_jerU.Pt());
	    l.FillTree("j3_jerU_phi",(float)j3_jerU.Phi());
    	l.FillTree("j3_jerU_eta",(float)j3_jerU.Eta());
	    //l.FillTree("j3_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[2]]);
	    l.FillTree("j3_beta", (float)l.jet_algoPF1_beta[jets[2]]);
	    l.FillTree("j3_betaStar", (float)l.jet_algoPF1_betaStar[jets[2]]);
	    l.FillTree("j3_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[2]]);
	    l.FillTree("j3_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[2]]);
        l.FillTree("j3_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[2]]);
        l.FillTree("j3_csvMvaBtag", (float)l.jet_algoPF1_csvMvaBtag[jets[2]]);
        l.FillTree("j3_jetProbBtag", (float)l.jet_algoPF1_jetProbBtag[jets[2]]);
        l.FillTree("j3_tcheBtag", (float)l.jet_algoPF1_tcheBtag[jets[2]]);
        //BtagSF variables (Badder)
        if(l.itype[l.current] == -301 || l.itype[l.current] == -501 || l.itype[l.current] == -701 || l.itype[l.current] == -1001 || l.itype[l.current] == -1501){
           if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling BtagSF variables, jet3" << endl;
           int flavour = jetFlavReader->getJetFlavour((int)l.lumis, (int)l.event,jet3); 
           float btagSF = SFReader->getSF(jet3,flavour,l.jet_algoPF1_csvBtag[jets[2]]);
           float btagSFErrorUp = SFReader->getSFErrorUp(jet3,flavour,l.jet_algoPF1_csvBtag[jets[2]]);
           float btagSFErrorDown = SFReader->getSFErrorDown(jet3,flavour,l.jet_algoPF1_csvBtag[jets[2]]);
           float btagEff = EffReader->getBtagEfficiency(jet3,l.jet_algoPF1_csvBtag[jets[2]],flavour);
           float btagEffError = EffReader->getBtagEfficiencyError(jet3,l.jet_algoPF1_csvBtag[jets[2]],flavour);
           l.FillTree("j3_flavour",(int)flavour);
           l.FillTree("j3_btagSF",(float)btagSF);
           l.FillTree("j3_btagSFErrorUp",(float)btagSFErrorUp);
           l.FillTree("j3_btagSFErrorDown",(float)btagSFErrorDown);
           l.FillTree("j3_btagEff",(float)btagEff);
           l.FillTree("j3_btagEffError",(float)btagEffError);
        }else{
           l.FillTree("j3_flavour",(int)0);
           l.FillTree("j3_btagSF",(float)-1001.);
           l.FillTree("j3_btagSFErrorUp",(float)-1001.);
           l.FillTree("j3_btagSFErrorDown",(float)-1001.);
           l.FillTree("j3_btagEff",(float)-1001.);
           l.FillTree("j3_btagEffError",(float)-1001.);
        }
        l.FillTree("j3_bgenMatched", (float)l.jet_algoPF1_bgenMatched[jets[2]]);
		l.FillTree("j3_nSecondaryVertices", (float)l.jet_algoPF1_nSecondaryVertices[jets[2]]);
		l.FillTree("j3_secVtxPt", (float)l.jet_algoPF1_secVtxPt[jets[2]]);
		l.FillTree("j3_secVtx3dL", (float)l.jet_algoPF1_secVtx3dL[jets[2]]);
		l.FillTree("j3_secVtx3deL", (float)l.jet_algoPF1_secVtx3deL[jets[2]]);
		l.FillTree("j3_emfrac", (float)l.jet_algoPF1_emfrac[jets[2]]);
		l.FillTree("j3_hadfrac", (float)l.jet_algoPF1_hadfrac[jets[2]]);
		l.FillTree("j3_ntk", (int)l.jet_algoPF1_ntk[jets[2]]);
		l.FillTree("j3_nNeutrals", (int)l.jet_algoPF1_nNeutrals[jets[2]]);
		l.FillTree("j3_nCharged", (int)l.jet_algoPF1_nCharged[jets[2]]);
		l.FillTree("j3_genPt", (float)l.jet_algoPF1_genPt[jets[2]]);
    } else {
    	l.FillTree("j3_e",(float)-1001.);
	    l.FillTree("j3_pt",(float)-1001.);
    	l.FillTree("j3_phi",(float)-1001.);
    	l.FillTree("j3_eta",(float)-1001.);
	    l.FillTree("j3_jecD_e",(float)-1001.);
    	l.FillTree("j3_jecD_pt",(float)-1001.);
	    l.FillTree("j3_jecD_phi",(float)-1001.);
    	l.FillTree("j3_jecD_eta",(float)-1001.);
	    l.FillTree("j3_jecU_e",(float)-1001.);
    	l.FillTree("j3_jecU_pt",(float)-1001.);
	    l.FillTree("j3_jecU_phi",(float)-1001.);
    	l.FillTree("j3_jecU_eta",(float)-1001.);
	    l.FillTree("j3_jerD_e",(float)-1001.);
    	l.FillTree("j3_jerD_pt",(float)-1001.);
	    l.FillTree("j3_jerD_phi",(float)-1001.);
    	l.FillTree("j3_jerD_eta",(float)-1001.);
	    l.FillTree("j3_jerC_e",(float)-1001.);
    	l.FillTree("j3_jerC_pt",(float)-1001.);
	    l.FillTree("j3_jerC_phi",(float)-1001.);
    	l.FillTree("j3_jerC_eta",(float)-1001.);
	    l.FillTree("j3_jerU_e",(float)-1001.);
    	l.FillTree("j3_jerU_pt",(float)-1001.);
	    l.FillTree("j3_jerU_phi",(float)-1001.);
    	l.FillTree("j3_jerU_eta",(float)-1001.);
	    //l.FillTree("j3_cutbased_wp_level", (float)-1001.);
	    l.FillTree("j3_beta", (float)-1001.);
	    l.FillTree("j3_betaStar", (float)-1001.);
	    l.FillTree("j3_betaStarClassic", (float)-1001.);
	    l.FillTree("j3_dR2Mean", (float)-1001.);
        l.FillTree("j3_csvBtag", (float)-1001.);
        l.FillTree("j3_csvMvaBtag", (float)-1001.);
        l.FillTree("j3_jetProbBtag", (float)-1001.);
        l.FillTree("j3_tcheBtag", (float)-1001.);
        l.FillTree("j3_flavour",(int)0);
        l.FillTree("j3_btagSF",(float)-1001.);
        l.FillTree("j3_btagSFErrorUp",(float)-1001.);
        l.FillTree("j3_btagSFErrorDown",(float)-1001.);
        l.FillTree("j3_btagEff",(float)-1001.);
        l.FillTree("j3_btagEffError",(float)-1001.);
        l.FillTree("j3_bgenMatched", (float)-1001.);
		l.FillTree("j3_nSecondaryVertices", (float)-1001.);
		l.FillTree("j3_secVtxPt", (float)-1001.);
		l.FillTree("j3_secVtx3dL", (float)-1001.);
		l.FillTree("j3_secVtx3deL", (float)-1001.);
		l.FillTree("j3_emfrac", (float)-1001.);
		l.FillTree("j3_hadfrac", (float)-1001.);
		l.FillTree("j3_ntk", (int)-1001);
		l.FillTree("j3_nNeutrals", (int)-1001);
		l.FillTree("j3_nCharged", (int)-1001);
		l.FillTree("j3_genPt", (float)-1001);
    } // if 3 jets

    if(jets.size() > 3){
        if(PADEBUG) cout << "processing jet 3" << endl;
        jet4 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]);
    	l.FillTree("j4_e",(float)jet4->Energy());
	    l.FillTree("j4_pt",(float)jet4->Pt());
	    l.FillTree("j4_phi",(float)jet4->Phi());
	    l.FillTree("j4_eta",(float)jet4->Eta());
        TLorentzVector j4_jecD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]), jets[3], 1, -1., 0,  0.);
        TLorentzVector j4_jecU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]), jets[3], 1, +1., 0,  0.);
        TLorentzVector j4_jerD = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]), jets[3], 0,  0., 1, -1.);
        TLorentzVector j4_jerC = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]), jets[3], 0,  0., 1,  0.);
        TLorentzVector j4_jerU = getJecJer(l, (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]), jets[3], 0,  0., 1, +1.);
	    l.FillTree("j4_jecD_e",(float)j4_jecD.Energy());
    	l.FillTree("j4_jecD_pt",(float)j4_jecD.Pt());
	    l.FillTree("j4_jecD_phi",(float)j4_jecD.Phi());
    	l.FillTree("j4_jecD_eta",(float)j4_jecD.Eta());
	    l.FillTree("j4_jecU_e",(float)j4_jecU.Energy());
    	l.FillTree("j4_jecU_pt",(float)j4_jecU.Pt());
	    l.FillTree("j4_jecU_phi",(float)j4_jecU.Phi());
    	l.FillTree("j4_jecU_eta",(float)j4_jecU.Eta());
	    l.FillTree("j4_jerD_e",(float)j4_jerD.Energy());
    	l.FillTree("j4_jerD_pt",(float)j4_jerD.Pt());
	    l.FillTree("j4_jerD_phi",(float)j4_jerD.Phi());
    	l.FillTree("j4_jerD_eta",(float)j4_jerD.Eta());
	    l.FillTree("j4_jerC_e",(float)j4_jerC.Energy());
    	l.FillTree("j4_jerC_pt",(float)j4_jerC.Pt());
	    l.FillTree("j4_jerC_phi",(float)j4_jerC.Phi());
    	l.FillTree("j4_jerC_eta",(float)j4_jerC.Eta());
	    l.FillTree("j4_jerU_e",(float)j4_jerU.Energy());
    	l.FillTree("j4_jerU_pt",(float)j4_jerU.Pt());
	    l.FillTree("j4_jerU_phi",(float)j4_jerU.Phi());
    	l.FillTree("j4_jerU_eta",(float)j4_jerU.Eta());
	    //l.FillTree("j4_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[3]]);
	    l.FillTree("j4_beta", (float)l.jet_algoPF1_beta[jets[3]]);
	    l.FillTree("j4_betaStar", (float)l.jet_algoPF1_betaStar[jets[3]]);
	    l.FillTree("j4_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[3]]);
	    l.FillTree("j4_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[3]]);
        l.FillTree("j4_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[3]]);
        l.FillTree("j4_csvMvaBtag", (float)l.jet_algoPF1_csvMvaBtag[jets[3]]);
        l.FillTree("j4_jetProbBtag", (float)l.jet_algoPF1_jetProbBtag[jets[3]]);
        l.FillTree("j4_tcheBtag", (float)l.jet_algoPF1_tcheBtag[jets[3]]);
        //BtagSF variables (Badder)
        if(l.itype[l.current] == -301 || l.itype[l.current] == -501 || l.itype[l.current] == -701 || l.itype[l.current] == -1001 || l.itype[l.current] == -1501){
           if(PADEBUG) cerr << "StatAnalysis::fillOpTree: filling BtagSF variables, jet4" << endl;
           int flavour = jetFlavReader->getJetFlavour((int)l.lumis, (int)l.event,jet4); 
           float btagSF = SFReader->getSF(jet4,flavour,l.jet_algoPF1_csvBtag[jets[3]]);
           float btagSFErrorUp = SFReader->getSFErrorUp(jet4,flavour,l.jet_algoPF1_csvBtag[jets[3]]);
           float btagSFErrorDown = SFReader->getSFErrorDown(jet4,flavour,l.jet_algoPF1_csvBtag[jets[3]]);
           float btagEff = EffReader->getBtagEfficiency(jet4,l.jet_algoPF1_csvBtag[jets[3]],flavour);
           float btagEffError = EffReader->getBtagEfficiencyError(jet4,l.jet_algoPF1_csvBtag[jets[3]],flavour);
           l.FillTree("j4_flavour",(int)flavour);
           l.FillTree("j4_btagSF",(float)btagSF);
           l.FillTree("j4_btagSFErrorUp",(float)btagSFErrorUp);
           l.FillTree("j4_btagSFErrorDown",(float)btagSFErrorDown);
           l.FillTree("j4_btagEff",(float)btagEff);
           l.FillTree("j4_btagEffError",(float)btagEffError);
        }else{
           l.FillTree("j4_flavour",(int)0);
           l.FillTree("j4_btagSF",(float)-1001.);
           l.FillTree("j4_btagSFErrorUp",(float)-1001.);
           l.FillTree("j4_btagSFErrorDown",(float)-1001.);
           l.FillTree("j4_btagEff",(float)-1001.);
           l.FillTree("j4_btagEffError",(float)-1001.);
        }
        l.FillTree("j4_bgenMatched", (float)l.jet_algoPF1_bgenMatched[jets[3]]);
		l.FillTree("j4_nSecondaryVertices", (float)l.jet_algoPF1_nSecondaryVertices[jets[3]]);
		l.FillTree("j4_secVtxPt", (float)l.jet_algoPF1_secVtxPt[jets[3]]);
		l.FillTree("j4_secVtx3dL", (float)l.jet_algoPF1_secVtx3dL[jets[3]]);
		l.FillTree("j4_secVtx3deL", (float)l.jet_algoPF1_secVtx3deL[jets[3]]);
		l.FillTree("j4_emfrac", (float)l.jet_algoPF1_emfrac[jets[3]]);
		l.FillTree("j4_hadfrac", (float)l.jet_algoPF1_hadfrac[jets[3]]);
		l.FillTree("j4_ntk", (int)l.jet_algoPF1_ntk[jets[3]]);
		l.FillTree("j4_nNeutrals", (int)l.jet_algoPF1_nNeutrals[jets[3]]);
		l.FillTree("j4_nCharged", (int)l.jet_algoPF1_nCharged[jets[3]]);
		l.FillTree("j4_genPt", (float)l.jet_algoPF1_genPt[jets[3]]);
    } else {
    	l.FillTree("j4_e",(float)-1001.);
	    l.FillTree("j4_pt",(float)-1001.);
	    l.FillTree("j4_phi",(float)-1001.);
	    l.FillTree("j4_eta",(float)-1001.);
	    l.FillTree("j4_jecD_e",(float)-1001.);
    	l.FillTree("j4_jecD_pt",(float)-1001.);
	    l.FillTree("j4_jecD_phi",(float)-1001.);
    	l.FillTree("j4_jecD_eta",(float)-1001.);
	    l.FillTree("j4_jecU_e",(float)-1001.);
    	l.FillTree("j4_jecU_pt",(float)-1001.);
	    l.FillTree("j4_jecU_phi",(float)-1001.);
    	l.FillTree("j4_jecU_eta",(float)-1001.);
	    l.FillTree("j4_jerD_e",(float)-1001.);
    	l.FillTree("j4_jerD_pt",(float)-1001.);
	    l.FillTree("j4_jerD_phi",(float)-1001.);
    	l.FillTree("j4_jerD_eta",(float)-1001.);
	    l.FillTree("j4_jerC_e",(float)-1001.);
    	l.FillTree("j4_jerC_pt",(float)-1001.);
	    l.FillTree("j4_jerC_phi",(float)-1001.);
    	l.FillTree("j4_jerC_eta",(float)-1001.);
	    l.FillTree("j4_jerU_e",(float)-1001.);
    	l.FillTree("j4_jerU_pt",(float)-1001.);
	    l.FillTree("j4_jerU_phi",(float)-1001.);
    	l.FillTree("j4_jerU_eta",(float)-1001.);
	    //l.FillTree("j4_cutbased_wp_level", (float)-1001.);
	    l.FillTree("j4_beta", (float)-1001.);
	    l.FillTree("j4_betaStar", (float)-1001.);
	    l.FillTree("j4_betaStarClassic", (float)-1001.);
	    l.FillTree("j4_dR2Mean", (float)-1001.);
        l.FillTree("j4_csvBtag", (float)-1001.);
        l.FillTree("j4_csvMvaBtag", (float)-1001.);
        l.FillTree("j4_jetProbBtag", (float)-1001.);
        l.FillTree("j4_tcheBtag", (float)-1001.);
        l.FillTree("j4_flavour",(int)0);
        l.FillTree("j4_btagSF",(float)-1001.);
        l.FillTree("j4_btagSFErrorUp",(float)-1001.);
        l.FillTree("j4_btagSFErrorDown",(float)-1001.);
        l.FillTree("j4_btagEff",(float)-1001.);
        l.FillTree("j4_btagEffError",(float)-1001.);
        l.FillTree("j4_bgenMatched", (float)-1001.);
		l.FillTree("j4_nSecondaryVertices", (float)-1001.);
		l.FillTree("j4_secVtxPt", (float)-1001.);
		l.FillTree("j4_secVtx3dL", (float)-1001.);
		l.FillTree("j4_secVtx3deL", (float)-1001.);
		l.FillTree("j4_emfrac", (float)-1001.);
		l.FillTree("j4_hadfrac", (float)-1001.);
		l.FillTree("j4_ntk", (int)-1001);
		l.FillTree("j4_nNeutrals", (int)-1001);
		l.FillTree("j4_nCharged", (int)-1001);
		l.FillTree("j4_genPt", (float)-1001);
    } // if 4 jets

// MC Truth radion signal information
    if(l.itype[l.current] < -250)
    {
	    TLorentzVector * radion = (TLorentzVector *)l.gr_radion_p4->At(0);
	    TLorentzVector * hgg = (TLorentzVector *)l.gr_hgg_p4->At(0);
	    TLorentzVector * hbb = (TLorentzVector *)l.gr_hbb_p4->At(0);
	    TLorentzVector * mcg1 = (TLorentzVector *)l.gr_g1_p4->At(0);
	    TLorentzVector * mcg2 = (TLorentzVector *)l.gr_g2_p4->At(0);
	    TLorentzVector * mcb1 = (TLorentzVector *)l.gr_b1_p4->At(0);
	    TLorentzVector * mcb2 = (TLorentzVector *)l.gr_b2_p4->At(0);
	    TLorentzVector * mcj1 = (TLorentzVector *)l.gr_j1_p4->At(0);
	    TLorentzVector * mcj2 = (TLorentzVector *)l.gr_j2_p4->At(0);
	
		l.FillTree("gr_radion_p4_pt", (float)radion->Pt());
		l.FillTree("gr_radion_p4_eta", (float)radion->Eta());
		l.FillTree("gr_radion_p4_phi", (float)radion->Phi());
		l.FillTree("gr_radion_p4_mass", (float)radion->M());
		l.FillTree("gr_hgg_p4_pt", (float)hgg->Pt());
		l.FillTree("gr_hgg_p4_eta", (float)hgg->Eta());
		l.FillTree("gr_hgg_p4_phi", (float)hgg->Phi());
		l.FillTree("gr_hgg_p4_mass", (float)hgg->M());
		l.FillTree("gr_hbb_p4_pt", (float)hbb->Pt());
		l.FillTree("gr_hbb_p4_eta", (float)hbb->Eta());
		l.FillTree("gr_hbb_p4_phi", (float)hbb->Phi());
		l.FillTree("gr_hbb_p4_mass", (float)hbb->M());
		l.FillTree("gr_g1_p4_pt", (float)mcg1->Pt());
		l.FillTree("gr_g1_p4_eta", (float)mcg1->Eta());
		l.FillTree("gr_g1_p4_phi", (float)mcg1->Phi());
		l.FillTree("gr_g1_p4_mass", (float)mcg1->M());
		l.FillTree("gr_g2_p4_pt", (float)mcg2->Pt());
		l.FillTree("gr_g2_p4_eta", (float)mcg2->Eta());
		l.FillTree("gr_g2_p4_phi", (float)mcg2->Phi());
		l.FillTree("gr_g2_p4_mass", (float)mcg2->M());
		l.FillTree("gr_b1_p4_pt", (float)mcb1->Pt());
		l.FillTree("gr_b1_p4_eta", (float)mcb1->Eta());
		l.FillTree("gr_b1_p4_phi", (float)mcb1->Phi());
		l.FillTree("gr_b1_p4_mass", (float)mcb1->M());
		l.FillTree("gr_b2_p4_pt", (float)mcb2->Pt());
		l.FillTree("gr_b2_p4_eta", (float)mcb2->Eta());
		l.FillTree("gr_b2_p4_phi", (float)mcb2->Phi());
		l.FillTree("gr_b2_p4_mass", (float)mcb2->M());
		l.FillTree("gr_j1_p4_pt", (float)mcj1->Pt());
		l.FillTree("gr_j1_p4_eta", (float)mcj1->Eta());
		l.FillTree("gr_j1_p4_phi", (float)mcj1->Phi());
		l.FillTree("gr_j1_p4_mass", (float)mcj1->M());
		l.FillTree("gr_j2_p4_pt", (float)mcj2->Pt());
		l.FillTree("gr_j2_p4_eta", (float)mcj2->Eta());
		l.FillTree("gr_j2_p4_phi", (float)mcj2->Phi());
		l.FillTree("gr_j2_p4_mass", (float)mcj2->M());
    } else {
		l.FillTree("gr_radion_p4_pt", (float)-1001.);
		l.FillTree("gr_radion_p4_eta", (float)-1001.);
		l.FillTree("gr_radion_p4_phi", (float)-1001.);
		l.FillTree("gr_radion_p4_mass", (float)-1001.);
		l.FillTree("gr_hgg_p4_pt", (float)-1001.);
		l.FillTree("gr_hgg_p4_eta", (float)-1001.);
		l.FillTree("gr_hgg_p4_phi", (float)-1001.);
		l.FillTree("gr_hgg_p4_mass", (float)-1001.);
		l.FillTree("gr_hbb_p4_pt", (float)-1001.);
		l.FillTree("gr_hbb_p4_eta", (float)-1001.);
		l.FillTree("gr_hbb_p4_phi", (float)-1001.);
		l.FillTree("gr_hbb_p4_mass", (float)-1001.);
		l.FillTree("gr_g1_p4_pt", (float)-1001.);
		l.FillTree("gr_g1_p4_eta", (float)-1001.);
		l.FillTree("gr_g1_p4_phi", (float)-1001.);
		l.FillTree("gr_g1_p4_mass", (float)-1001.);
		l.FillTree("gr_g2_p4_pt", (float)-1001.);
		l.FillTree("gr_g2_p4_eta", (float)-1001.);
		l.FillTree("gr_g2_p4_phi", (float)-1001.);
		l.FillTree("gr_g2_p4_mass", (float)-1001.);
		l.FillTree("gr_b1_p4_pt", (float)-1001.);
		l.FillTree("gr_b1_p4_eta", (float)-1001.);
		l.FillTree("gr_b1_p4_phi", (float)-1001.);
		l.FillTree("gr_b1_p4_mass", (float)-1001.);
		l.FillTree("gr_b2_p4_pt", (float)-1001.);
		l.FillTree("gr_b2_p4_eta", (float)-1001.);
		l.FillTree("gr_b2_p4_phi", (float)-1001.);
		l.FillTree("gr_b2_p4_mass", (float)-1001.);
		l.FillTree("gr_j1_p4_pt", (float)-1001.);
		l.FillTree("gr_j1_p4_eta", (float)-1001.);
		l.FillTree("gr_j1_p4_phi", (float)-1001.);
		l.FillTree("gr_j1_p4_mass", (float)-1001.);
		l.FillTree("gr_j2_p4_pt", (float)-1001.);
		l.FillTree("gr_j2_p4_eta", (float)-1001.);
		l.FillTree("gr_j2_p4_phi", (float)-1001.);
		l.FillTree("gr_j2_p4_mass", (float)-1001.);
    } // end if type is signal

    // fill photon systematics

    TLorentzVector lead_pesD, lead_pesU, lead_perD, lead_perU;
    TLorentzVector sublead_pesD, sublead_pesU, sublead_perD, sublead_perU;
    int cur_type = l.itype[l.current];
        for(std::vector<BaseSmearer *>::iterator  si=systPhotonSmearers_.begin(); si!= systPhotonSmearers_.end(); ++si ) {
            if( (*si)->name() != "E_scale" && (*si)->name() != "E_res") continue;
            if(PADEBUG) cout << "(*si)->name()= " << (*si)->name() << "\t(*si)->nRegisteredSmerers()= " << (*si)->nRegisteredSmerers() << endl;
            float systStep = 3.0;
            for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                if( syst_shift == 0. ) { continue; } // skip the central value
        applySinglePhotonSmearings(smeared_pho_energy, smeared_pho_r9, smeared_pho_weight, cur_type, l, energyCorrected, energyCorrectedError,
                *si, syst_shift);
        if(PADEBUG) cout << "smeared_pho_energy[" << diphoton_index.first << "]= " << smeared_pho_energy[diphoton_index.first] << endl;
        if(PADEBUG) cout << "smeared_pho_energy[" << diphoton_index.second << "]= " << smeared_pho_energy[diphoton_index.second] << endl;
        if( (*si)->name() == "E_scale" && syst_shift < 0.)
        {
            lead_pesD = l.get_pho_p4(diphoton_index.first, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
            sublead_pesD = l.get_pho_p4(diphoton_index.second, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
        }
        else if( (*si)->name() == "E_scale" && syst_shift > 0.)
        {
            lead_pesU = l.get_pho_p4(diphoton_index.first, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
            sublead_pesU = l.get_pho_p4(diphoton_index.second, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
        }
        else if( (*si)->name() == "E_res" && syst_shift < 0.)
        {
            lead_perD = l.get_pho_p4(diphoton_index.first, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
            sublead_perD = l.get_pho_p4(diphoton_index.second, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
        }
        else if( (*si)->name() == "E_res" && syst_shift > 0.)
        {
            lead_perU = l.get_pho_p4(diphoton_index.first, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
            sublead_perU = l.get_pho_p4(diphoton_index.second, (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]), &smeared_pho_energy[0]);
        }
            }
        }
        if(PADEBUG)
        {
	        cout << "lead (pt, eta, phi, e)= ( " << lead_p4.Pt() << " , " << lead_p4.Eta() << " , " << lead_p4.Phi() << " , " << lead_p4.E() << " )" << endl;
	        cout << "pesD (pt, eta, phi, e)= ( " << lead_pesD.Pt() << " , " << lead_pesD.Eta() << " , " << lead_pesD.Phi() << " , " << lead_pesD.E() << " )" << endl;
	        cout << "pesU (pt, eta, phi, e)= ( " << lead_pesU.Pt() << " , " << lead_pesU.Eta() << " , " << lead_pesU.Phi() << " , " << lead_pesU.E() << " )" << endl;
	        cout << "perD (pt, eta, phi, e)= ( " << lead_perD.Pt() << " , " << lead_perD.Eta() << " , " << lead_perD.Phi() << " , " << lead_perD.E() << " )" << endl;
	        cout << "perU (pt, eta, phi, e)= ( " << lead_perU.Pt() << " , " << lead_perU.Eta() << " , " << lead_perU.Phi() << " , " << lead_perU.E() << " )" << endl;
	        cout << "subl (pt, eta, phi, e)= ( " << sublead_p4.Pt() << " , " << sublead_p4.Eta() << " , " << sublead_p4.Phi() << " , " << sublead_p4.E() << " )" << endl;
	        cout << "pesD (pt, eta, phi, e)= ( " << sublead_pesD.Pt() << " , " << sublead_pesD.Eta() << " , " << sublead_pesD.Phi() << " , " << sublead_pesD.E() << " )" << endl;
	        cout << "pesU (pt, eta, phi, e)= ( " << sublead_pesU.Pt() << " , " << sublead_pesU.Eta() << " , " << sublead_pesU.Phi() << " , " << sublead_pesU.E() << " )" << endl;
	        cout << "perD (pt, eta, phi, e)= ( " << sublead_perD.Pt() << " , " << sublead_perD.Eta() << " , " << sublead_perD.Phi() << " , " << sublead_perD.E() << " )" << endl;
	        cout << "perU (pt, eta, phi, e)= ( " << sublead_perU.Pt() << " , " << sublead_perU.Eta() << " , " << sublead_perU.Phi() << " , " << sublead_perU.E() << " )" << endl;
        }

	    l.FillTree("ph1_pesD_pt",(float)lead_pesD.Pt());
	    l.FillTree("ph2_pesD_pt",(float)sublead_pesD.Pt());
	    l.FillTree("ph1_pesD_e",(float)lead_pesD.E());
	    l.FillTree("ph2_pesD_e",(float)sublead_pesD.E());
	    l.FillTree("ph1_pesU_pt",(float)lead_pesU.Pt());
	    l.FillTree("ph2_pesU_pt",(float)sublead_pesU.Pt());
	    l.FillTree("ph1_pesU_e",(float)lead_pesU.E());
	    l.FillTree("ph2_pesU_e",(float)sublead_pesU.E());
	    l.FillTree("ph1_perD_pt",(float)lead_perD.Pt());
	    l.FillTree("ph2_perD_pt",(float)sublead_perD.Pt());
	    l.FillTree("ph1_perD_e",(float)lead_perD.E());
	    l.FillTree("ph2_perD_e",(float)sublead_perD.E());
	    l.FillTree("ph1_perU_pt",(float)lead_perU.Pt());
	    l.FillTree("ph2_perU_pt",(float)sublead_perU.Pt());
	    l.FillTree("ph1_perU_e",(float)lead_perU.E());
	    l.FillTree("ph2_perU_e",(float)sublead_perU.E());


    if(PADEBUG) cerr << "Leaving StatAnalysis::fillOpTree" << endl;
};


// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
