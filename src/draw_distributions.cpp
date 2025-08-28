#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1D.h"
#include "THStack.h"
#include "TCanvas.h"
#include "TStyle.h"

#include <utility>
#include <iostream>
#include <map>
#include <limits>

int main(int argc, const char* argv[]){
	if(argc<3){
		std::cout<<"usage: "<<argv[0]<<" <output file> <input file 1> <input file 2> ..."<<std::endl;
		return 0;
	}
	
	std::string output_file = argv[1];
	TFile* fo = new TFile(output_file.c_str(),"RECREATE");
	if(!fo || fo->IsZombie()){
		std::cerr<<"couldn't open output file '"<<output_file<<"'"<<std::endl;
		return -1;
	}
	
	TFile* f = nullptr;
	TTree* t = nullptr;
	if(true || argc==2){
		
		std::string input_file = argv[2];
		f = TFile::Open(input_file.c_str(),"READ");
		if(!f || f->IsZombie()){
			std::cerr<<"couldn't open input file '"<<input_file<<"'"<<std::endl;
			return -1;
		}
		t = (TTree*)f->Get("vals");
		if(!t){
			std::cerr<<"no 'vals' tree in input file"<<std::endl;
			return -1;
		}
		
	} else {
		
		TChain* c = new TChain("vals");
		for(int i=2; i<argc-2; ++i){
			std::cout<<"adding file "<<argv[i]<<std::endl;
			int ntrees = c->Add(argv[i]);
			if(ntrees!=i-2) std::cout<<"files in chain: "<<ntrees<<", files added: "<<(i-2)<<std::endl;
		}
		t = dynamic_cast<TTree*>(c);
		// not sure if this will work as the number of entries in each branch
		// is different. Still, the number of entries in the tree ought to be set
		// to the maximum number of entries in a given branch.
		int localEntry = t->LoadTree(0);
		if(localEntry<0){
			std::cerr<<"No entries in vals chain!"<<std::endl;
			return -1;
		}
		
	}
	
	if(t->GetListOfBranches()->GetEntries()==0){
		std::cerr<<"'vals' tree has no branches!"<<std::endl;
		return -1;
	}
	
	std::map<std::string, TH1D*> histsvec;
	
	// first time just make histograms of all events
	// so we accumuate all events and make binning
	for(int htype=0; htype<2; ++htype){
		for(int filei=2; filei<argc; ++filei){
			std::cout<<"adding entries from file "<<argv[filei]<<std::endl;
			
			// first file already opened, but open any subsequent files
			if(htype>0 || filei>2){
				f = TFile::Open(argv[filei],"READ");
				t = (TTree*)f->Get("vals");
				if(!f || f->IsZombie()){
					std::cerr<<"couldn't open input file '"<<argv[filei]<<"'"<<std::endl;
					//return -1;
					continue;
				}
				t = (TTree*)f->Get("vals");
				if(!t){
					std::cerr<<"no 'vals' tree in input file '"<<argv[filei]<<std::endl;
					//return -1;
					continue;
				}
				
			}
			
			fo->cd();
			
			for(int i=0; i<t->GetListOfBranches()->GetEntries(); ++i){
				TBranch* b = (TBranch*)(t->GetListOfBranches()->At(i));
				std::string bname = b->GetName();
				
				//FIXME remove FIXME
				//if(bname!="relic_mu_tdiff") continue;
				
				if(bname.length()>5){
					std::string tail = bname.substr(bname.length()-5,std::string::npos);
					if(tail=="_pass"){
						// skip pass branch, process pairs
						continue;
					}
				}
				std::string pname = bname+"_pass";
				TBranch* bp = (TBranch*)t->GetBranch(pname.c_str());
				if(bp==nullptr){
					std::cerr<<"couldn't find pass branch "<<(pname)<<std::endl;
					continue;
				}
				size_t nentries = b->GetEntries();
				
				std::cout<<"branch "<<bname<<" has "<<nentries<<" entries"<<std::endl;
				if(nentries==0){
					//std::cerr<<"warning; branch "<<bname<<" has no entries"<<std::endl;
					// not necesarily problematic as some cuts are boolean so have no distribution to plot
					continue;
				}
				if((bp!=nullptr) && (bp->GetEntries()!=nentries)){
					std::cerr<<"mismatch in number of entries between branch "<<bname<<" and "<<pname
					         <<"; "<<nentries<<" vs "<<bp->GetEntries()<<" respectively"<<std::endl;
					continue;
				}
				double tmpval; // FIXME get the right type
				int ret = t->SetBranchAddress(bname.c_str(),&tmpval);
				if(ret<0){
					std::cerr<<"error "<<ret<<" setting address for branch "<<bname<<std::endl;
					continue;
				} else if(ret!=0){
					std::cerr<<"warning "<<ret<<" setting address for branch "<<bname<<std::endl;
					// some kind of conversion required...
				}
				bool tmppass;
				ret = t->SetBranchAddress(pname.c_str(),&tmppass);
				if(ret<0){
					std::cerr<<"error "<<ret<<" setting address for branch "<<pname<<std::endl;
					continue;
				} else if(ret!=0){
					std::cerr<<"warning "<<ret<<" setting address for branch "<<pname<<std::endl;
					// some kind of conversion required...
				}
				
				// we need to fill just this first in order to determine binning
				// otherwise binning of the pass histo is not the same as it doesn't
				// see all events, and if binning is different we can't compare them.
				if(histsvec.count(bname)==0){
					std::cout<<"New hist: "<<bname<<std::endl;
					histsvec.emplace(bname, new TH1D(bname.c_str(),bname.c_str(),200,0.,0.));
				}
				TH1D* bhist=histsvec.at(bname);
				TH1D* phist=nullptr;
				
				if(htype==0){
					for(int j=0; j<nentries; ++j){
						b->GetEntry(j);
						if(bname=="BSenergy" && tmpval==9999 ||
						   tmpval==std::numeric_limits<double>::infinity()) continue; // this f#&*s the histogram limits
						if(bname=="relic_mu_tdiff" && tmpval>70E3) continue; // maybe these are bad rollovers or sth
						// but we have a few way out here that mean we can't see the distribution properly
						bhist->Fill(tmpval);
					}
					
				} else {
					
					if(histsvec.count(pname)==0){
					    TH1D* phist = (TH1D*)bhist->Clone(pname.c_str());
					    phist->SetTitle(pname.c_str());
					    phist->Reset();
					    histsvec.emplace(pname, phist);
					}
					phist=histsvec.at(pname);
					for(int j=0; j<nentries; ++j){
						bp->GetEntry(j);
						if(tmppass){
							b->GetEntry(j);
							phist->Fill(tmpval);
						}
					}
				}
				
				if(filei==argc-1){
					std::cout<<"writing files"<<std::endl;
					if(htype==0) bhist->Write(bname.c_str());
					else phist->Write(pname.c_str());
				}
			}
			//fo->Write("*",TObject::kOverwrite);
			f->Close();
		}
	}
	
	
	TCanvas c1("c1","c1",1024,800);
	for(std::pair<const std::string,TH1D*>& nextentry : histsvec){
		std::string bname = nextentry.first;
		TH1D* ahist = nextentry.second;
		if(bname.find("_pass")!=std::string::npos) continue;
		std::string stackname = bname+"_stack";
		THStack hs(stackname.c_str(),stackname.c_str());
		ahist->SetLineColor(kRed);
		ahist->SetFillColor(kRed);
		ahist->SetFillStyle(3003);
		hs.Add(ahist);
		TH1D* phist = histsvec.at(bname+"_pass");
		phist->SetLineColor(kSpring-1);
		phist->SetFillColor(kSpring-1);
		phist->SetFillStyle(3003);
		hs.Add(phist);
		c1.Clear();
		hs.Draw("nostack");
		std::string canvname = "c_"+bname;
		c1.SetName(canvname.c_str());
		c1.Write();
	}
	
	fo->Close();
	
	return 0;
}
