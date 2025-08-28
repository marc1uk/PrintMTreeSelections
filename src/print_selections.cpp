#include "MTreeReader.h"
#include "MTreeSelection.h"

#include "TParameter.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TKey.h"

#include "SkrootHeaders.h"    // MCInfo, Header etc.

#include <cstring>

int main(int argc, const char** argv){
	if(argc<2){
		std::cout<<"usage: "<<argv[0]<<" "<<"<selectionsfile> <rawfile>"<<std::endl;
		return 0;
	}
	std::string cutFile = argv[1];
	
	TFile* f = TFile::Open(cutFile.c_str());
	if(!f || f->IsZombie()){
		std::cerr<<"couldn't open selections file "<<cutFile<<std::endl;
		return 1;
	}
	TObjArray* cut_tracker_obj = (TObjArray*)f->Get("cut_tracker");
	//TObjArray* cut_order_obj = (TObjArray*)f->Get("cut_order");
	if(!cut_tracker_obj /*|| !cut_order_obj*/){
		std::cerr<<"couldn't get 'cut_tracker'"/*<<" or 'cut_order'"<<*/" from file"<<std::endl;
		return 1;
	}
	
//	static char cmd[std::strlen("/usr/bin/gdb --nx --batch --quiet -ex 'set confirm off' -ex 'attach XXXXX' -ex 'thread apply all bt full' -ex quit")];
//	std::sprintf(cmd, "/usr/bin/gdb --nx --batch --quiet -ex 'set confirm off' -ex 'attach %u' -ex 'thread apply all bt full' -ex quit", getpid());
//	std::system(cmd);
	
	std::map<std::string, uint64_t> cut_tracker;
	std::vector<std::string> cut_order;
	
	/* 
	std::cout<<"getting names of cuts"<<std::endl;
	for(int cut_i=0; cut_i<cut_order_obj->GetEntries(); ++cut_i){
		TObjString* current_cut = (TObjString*)cut_order_obj->At(cut_i);
		std::cout<<"next cut: "<<current_cut<<std::flush;
		std::cout<<"of name: "<<current_cut->GetName()<<std::endl;
		cut_order.push_back(current_cut->GetName());
	}
	*/
	
	std::cout<<"getting cuts in order of application"<<std::endl;
	for(int cut_i=0; cut_i<cut_tracker_obj->GetEntries(); ++cut_i){
		TParameter<Long64_t>* current_cut = (TParameter<Long64_t>*)cut_tracker_obj->At(cut_i);
		std::cout<<"next cut: "<<current_cut<<std::flush;
		std::cout<<"of name: "<<current_cut->GetName()<<std::endl;
		cut_order.push_back(current_cut->GetName());
		cut_tracker.emplace(current_cut->GetName(), current_cut->GetVal());
	}
	
	std::cout<<"number of passing entries"<<std::endl;
	for(int i=0; i<cut_order.size(); ++i){
		std::cout<<"cut "<<i<<": "<<cut_order.at(i)
		         <<" => "<<cut_tracker.at(cut_order.at(i))<<"\n";
	}
	
	std::cout<<"number of passing unique TTree entries"<<std::endl;
	for(int i=0; i<cut_order.size(); ++i){
		std::string entrylist_name = "TEntryList_"+cut_order.at(i);
		TEntryList* the_elist = (TEntryList*)f->Get(entrylist_name.c_str());
		Long64_t num_unique_events = the_elist->GetN();
		std::cout<<"cut "<<i<<": "<<cut_order.at(i)
		         <<" => "<<num_unique_events<<"\n";
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
//	         <<"afterwards we had "<<lowe_count_afer_lt_cut<<" events"<<std::endl;
	
	
	if(argc<3) return 0;
	std::string rawfile = argv[2];
	
	// check the date of the first and last event we analyse to check we have the correct run range
	TFile* fraw = TFile::Open(rawfile.c_str());
	if(fraw==nullptr){
		std::cout<<"Could not open "<<rawfile.c_str()<<std::endl;
		return 0;
	}
	TTree* traw = (TTree*)fraw->Get("data");
	if(!traw){
		std::cerr<<"No 'data' tree in file!"<<std::endl;
		std::map<std::string,TTree*> trees;
		for(int i=0; i<fraw->GetListOfKeys()->GetEntries(); ++i){
			TKey* key=(TKey*)fraw->GetListOfKeys()->At(i);
			TClass* cl=gROOT->GetClass(key->GetClassName());
			if(cl->InheritsFrom("TTree")){
				trees.emplace(key->GetName(),(TTree*)key->ReadObj());
			}
		}
		if(trees.size()==1){
			traw=trees.begin()->second;
			std::cout<<"using tree '"<<trees.begin()->first<<"'"<<std::endl;
		} else {
			while(true){
				std::cout<<"which tree to use?"<<std::endl;
				std::string choice;
				std::cin >> choice;
				if(trees.count(choice)==0){
					std::cout<<"not in map, try again"<<std::endl;
				} else {
					traw=trees.at(choice);
					break;
				}
			}
		}
	}
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
		std::cout<<"first passing entry was "<<first_entry_num<<", last passing entry was "<<last_entry_num<<std::endl;
		// get first passing entry data
		traw->GetEntry(first_entry_num);
		
		// first run start
		runstart.tm_year = HEADER->ndaysk[0];     // seems to already start at 1900? struct tm requires 1900-base.
		runstart.tm_mon = HEADER->ndaysk[1] - 1;  // not sure if this counts from 0 or 1. struct tm requires 0-base.
		runstart.tm_mday = HEADER->ndaysk[2];
		runstart.tm_hour = HEADER->ntimsk[0];
		runstart.tm_min = HEADER->ntimsk[1];
		runstart.tm_sec = HEADER->ntimsk[2];
		std::cout<<"start was run "
				 <<HEADER->nrunsk<<", on "
				 <<HEADER->ndaysk[0]+1900<<" years, "
				 <<HEADER->ndaysk[1]-1<<" months, "
				 <<HEADER->ndaysk[2]<<" days, "
				 <<HEADER->ntimsk[0]<<":"<<HEADER->ntimsk[1]<<":"<<HEADER->ntimsk[2]<<std::endl;
				
		
		// last run end
		traw->GetEntry(last_entry_num);
		runend.tm_year = HEADER->ndaysk[0];
		runend.tm_mon = HEADER->ndaysk[1] - 1;
		runend.tm_mday = HEADER->ndaysk[2];
		runend.tm_hour = HEADER->ntimsk[0];
		runend.tm_min = HEADER->ntimsk[1];
		runend.tm_sec = HEADER->ntimsk[2];
        std::cout<<"end was run "
        		 <<HEADER->nrunsk<<", on "
                 <<HEADER->ndaysk[0]+1900<<" years, "
                 <<HEADER->ndaysk[1]-1<<" months, "
	             <<HEADER->ndaysk[2]<<" days, "
	             <<HEADER->ntimsk[0]<<":"<<HEADER->ntimsk[1]<<":"<<HEADER->ntimsk[2]<<std::endl;
		
		//std::cout<<"runstart year is "<<runstart.tm_year<<", run end year is "<<runend.tm_year<<std::endl;
		// IMPORTANT: asctime holds an internal char array, and subsequent calls will overwrite it!
		// calling asctime twice in the same line can result in a race condition and incorrect results!
		std::string runstartstring = asctime(&runstart);
		std::string runendstring = asctime(&runend);
		std::cout<<"cut "<<cut_order.at(i)<<": first event at "<<runstartstring<<", last passing event at: "<<runendstring<<std::endl;
		// format should be <weekday> <month> <day> <hour:min:sec> <year>
		// seems like weekday comeso out as "???", but i guess we don't care anyway
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
