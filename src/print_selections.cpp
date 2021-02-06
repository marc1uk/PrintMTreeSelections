#include "MTreeReader.h"
#include "MTreeSelection.h"

#include "TParameter.h"
#include "TSystem.h"

#include "SkrootHeaders.h"    // MCInfo, Header etc.

int main(int argc, const char** argv){
	if(argc<2){
		std::cout<<"usage: "<<argv[0]<<" "<<"<filename>"<<std::endl;
		return 0;
	}
	std::string cutFile = argv[1];
	
	TFile* f = TFile::Open(cutFile.c_str());
	TObjArray* cut_tracker_obj = (TObjArray*)f->Get("cut_tracker");
	TObjArray* cut_order_obj = (TObjArray*)f->Get("cut_order");
	
	std::map<std::string, uint64_t> cut_tracker;
	std::vector<std::string> cut_order;
	for(int cut_i=0; cut_i<cut_order_obj->GetEntries(); ++cut_i){
		TObjString* current_cut = (TObjString*)cut_order_obj->At(cut_i);
		cut_order.push_back(current_cut->GetName());
	}
	for(int cut_i=0; cut_i<cut_tracker_obj->GetEntries(); ++cut_i){
		TParameter<Long64_t>* current_cut = (TParameter<Long64_t>*)cut_tracker_obj->At(cut_i);
		cut_tracker.emplace(current_cut->GetName(), current_cut->GetVal());
	}
	
	std::cout<<"counts, including splitting (one entry per matching muon)"<<std::endl;
	for(int i=0; i<cut_order.size(); ++i){
		std::cout<<((i==0) ? "\n" : "")<<"cut "<<i<<": "<<cut_order.at(i)
			 <<" => "<<cut_tracker.at(cut_order.at(i))<<"\n";
	}
	
	std::cout<<"counts, without splitting (one entry per lowe event)"<<std::endl;
	for(int i=0; i<cut_order.size(); ++i){
		std::string entrylist_name = "TEntryList_"+cut_order.at(i);
		TEntryList* the_elist = (TEntryList*)f->Get(entrylist_name.c_str());
		Long64_t num_lowe_events = the_elist->GetN();
		std::cout<<((i==0) ? "\n" : "")<<"cut "<<i<<": "<<cut_order.at(i)
			 <<" => "<<num_lowe_events<<"\n";
	}
	
//	// ok so this prints that we had 33370492 "events" before the lt<200cm cut
//	// and then had 1275433 after the lt cut. But this counts mu-lowe pairs,
//	// so how many unique lowe events does this represent? we can get that
//	// from the number of entries in the corresponding TEntryList or TTree
//	// first the number of mu-lowe pairs passing previous cuts
//	TEntryList* before_lt_cut = (TEntryList*)f->Get("TEntryList_muboy_index==0");
//	Long64_t lowe_count_before_lt_cut = before_lt_cut->GetN();
//	TEntryList* after_lt_cut = (TEntryList*)f->Get("TEntryList_dlt_mu_lowe>200cm");
//	Long64_t lowe_count_afer_lt_cut = after_lt_cut->GetN();
//	std::cout<<"before lt cut we had "<<lowe_count_before_lt_cut<<" events, "
//		 <<"afterwards we had "<<lowe_count_afer_lt_cut<<" events"<<std::endl;
	
	
	// check the date of the first and last event we analyse to check we have the correct run range
	std::string rawfile="/disk02/lowe8/relic_sk4/dec20/data/for_ntag/spall_resq_oldbdt_nlow1/relic.precut.leaf.ntag_oldbdt_nlow1.spall_new_resq.061525.077958.root";
	TFile* fraw = TFile::Open(rawfile.c_str());
	if(fraw==nullptr){
		std::cout<<"Could not open "<<rawfile.c_str()<<std::endl;
		return 0;
	}
	TTree* traw = (TTree*)fraw->Get("data");
	// set branch address to read HEAD branch for date
	Header* HEADER = new Header();
	traw->SetBranchAddress("HEADER",&HEADER);
	struct tm runstart;
	struct tm runend;
	
	for(int i=0; i<cut_order.size(); ++i){
		std::string entrylist_name = "TEntryList_"+cut_order.at(i);
		TEntryList* the_elist = (TEntryList*)f->Get(entrylist_name.c_str());
		// get first and last passing entries in this selection
		Long64_t first_entry_num = the_elist->GetEntry(0);
		Long64_t last_entry_num = the_elist->GetEntry(the_elist->GetN()-1);
		// get first passing entry data
		traw->GetEntry(first_entry_num);
		
		// first run start
		runstart.tm_year = HEADER->ndaysk[0] - 1900;
		runstart.tm_mon = HEADER->ndaysk[1] - 1;
		runstart.tm_mday = HEADER->ndaysk[2];
		runstart.tm_hour = HEADER->ntimsk[0];
		runstart.tm_min = HEADER->ntimsk[1];
		runstart.tm_sec = HEADER->ntimsk[2];
		
		// last run end
		traw->GetEntry(last_entry_num);
		runend.tm_year = HEADER->ndaysk[0] - 1900;
		runend.tm_mon = HEADER->ndaysk[1] - 1;
		runend.tm_mday = HEADER->ndaysk[2];
		runend.tm_hour = HEADER->ntimsk[0];
		runend.tm_min = HEADER->ntimsk[1];
		runend.tm_sec = HEADER->ntimsk[2];
		
		std::cout<<"cut "<<cut_order.at(i)<<": ";
		printf("Start: %s, End: %s\n", asctime(&runstart), asctime(&runend));
	}
	
	return 0;
	
	// i don't know why this is needed here but not in ToolAnalysis
	// actually, adding it didn't help...why still complaining?
	gSystem->Load("/host/software/stllibs/libRootStl.so");
	
	// make the MTreeSelection to read the TEntryList file
	MTreeSelection* myTreeSelections = new MTreeSelection(cutFile);
	myTreeSelections->PrintCuts();
	
	/*
	// Read data from the file...
	MTreeReader myTreeReader = new MTreeReader(inputFile, "data");
	uint64_t entry_number = myTreeSelections->GetNextEntry(topCutName);
	ReadEntryNtuple(entry_number);
	*/
	
	delete myTreeSelections;
	
}
