// Read in gosia2 input files and do a loop over a range of matrix elements,
// extracting and plotting the chisq as we go

#include "TFile.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TGraph2D.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

string convertInt( int number ) {
	
	stringstream ss;
	ss << number;
	return ss.str();
	
}

string convertFloat( float number ) {
	
	stringstream ss;
	ss << number;
	return ss.str();
	
}

int LookUpOldChisq( const vector<double>& vec, double dme, double tme ) {

	// Cycle through array and return index that matches matrix elements
	int index = -1;
	
	for ( int i = 0; i < int(vec.size()/5); i++ ) {
	
		if ( TMath::Abs( dme - vec[5*i+0] ) < 1E-8 ) {
			if ( TMath::Abs( tme - vec[5*i+1] ) < 1E-8 ) {

				index = i; // found it!
				break; // stop looking once we've found it!

			}
		}
			
	}

	return index; 

}

double ReadChiSqFromFile( string gosiaoutfile ) {

	// Open gosia output file for projectile or target. Get from input 
	ifstream g2out;
	g2out.open( gosiaoutfile.c_str(), ios::in );	
	if( !g2out.is_open() ) {
	
		cout << "Unable to open " << gosiaoutfile << endl;
		return 999;

	}
	
	// Search for chisq value in file
	bool flag = false;
	string line, tmp;
	string qry = "     *** CHISQ=";
	stringstream gosia_chisq (stringstream::in | stringstream::out);
	double chisq = 999;
 
	getline(g2out,line);
	while ( !g2out.eof() ) {
		getline( g2out, line );
		if( line.compare( 0, qry.size(), qry ) == 0 ) {
			gosia_chisq.str("");
			gosia_chisq.clear();
			gosia_chisq << line.substr( qry.size(), qry.size()+15 );
			gosia_chisq >> chisq;
			flag = true;
		}
	}
	
	g2out.close();
	
	if( !flag ) cout << "Couldn't find chisq value in " << gosiaoutfile << endl;
	
	return chisq;

}

int GetChiSq( string in_proj, string in_targ, double &chisq_proj, double &chisq_targ ) {
	
	string out_proj = in_proj.substr( 0, in_proj.find_last_of(".") );
	string out_targ = in_targ.substr( 0, in_targ.find_last_of(".") );
	out_proj += ".out";
	out_targ += ".out";

	string cmd = "gosia2 < " + in_proj;
	cmd.append(" > /dev/null 2>&1");

	if( system(NULL) ) system( cmd.c_str() );
	else {
		cout << "Cannot run system command\n";
		return 0;
	}
	
	chisq_proj = ReadChiSqFromFile( out_proj );
	chisq_targ = ReadChiSqFromFile( out_targ );

	return 1;
	
}

int IntegrateProjectile( string in_proj ) {

	string intifile = in_proj.substr( 0, in_proj.find_last_of(".") );
	intifile += ".INTI.inp";
	
	string cmd = "gosia < " + intifile;
	cmd.append(" > /dev/null 2>&1");
	
	ifstream inti;
	inti.open( intifile.c_str(), ios::in );
	if( !inti.is_open() ) return 0;
	else inti.close();
	
	if( system(NULL) ) system( cmd.c_str() );
	else {
		cout << "Cannot run system command\n";
		return 0;
	}

	return 1;
	
}

int WriteMatrixElementsToFile( string in_proj, string in_targ, double tme, double dme, int tme_index, int dme_index ) {

	string mename, litname;
	ofstream mefile;
	ifstream litfile;
	string cmd;
	double tmp;
	int index = 1;

	// Projectile matrix elements
	mename = in_proj.substr( 0, in_proj.find_last_of(".") );
	mename += ".bst";
	litname = mename + ".lit";

	mefile.open( mename.c_str(), ios::out );
	if( !mefile.is_open() ) return 1;

	litfile.open( litname.c_str(), ios::in );
	if( !litfile.is_open() ) return 1;
	
	litfile >> tmp; // read first me value
	
	while ( !litfile.eof() ) {
		if ( index == tme_index ) {
			mefile << tme << endl; // write current dme
			litfile >> tmp; // dump initial tme value and read next one
		}
		else if ( index == dme_index ) {
			mefile << dme << endl; // write current dme
			litfile >> tmp; // dump initial dme value and read next one
		}
		else {
			mefile << tmp << endl;	// write to file if there is a next value
			litfile >> tmp;	 // continue reading
		}
		index++; // increment matrix element index 		
	}
	
	mefile.close();
	litfile.close();

	// Target matrix elements, copy from backup file
	mename = in_targ.substr( 0, in_targ.find_last_of(".") );
	mename += ".bst";
	litname = mename + ".lit";

	litfile.open( litname.c_str(), ios::in );
	if( !litfile.is_open() ) return 2;
	else litfile.close();
	
	cmd = "cp " + litname + " " + mename;
	if( system(NULL) ) system( cmd.c_str() );
	else return 2;

	return 0;

}


void PrintUsage( char* progname ) {

	cout << "\nUsage: \n" << progname << " <in_proj> <in_targ> ";
	cout << "<Ndata_proj=3> <Ndata_targ=3> <low_TME=0.1> \n";
	cout << "   <upp_TME=2.5> <Nsteps_TME=51> <low_DME=0.0> <upp_DME=0.0> <Nsteps_DME=1>\n\n";
	cout << " where <Ndata_proj=3> and <Ndata_targ=5> are the number of data\n";
	cout << "for the projectile and target, respectively. This includes the\n";
	cout << "sum of all g-ray yields, matrix elements, lifetimes, etc.\n\n";
	cout << "Only <in_proj> and <in_targ>, the input projectile and target files ready\n";
	cout << "for minimisation, are required. All others have default values as \n";
	cout << "indicated in the usage.\n";
	cout << "If \"cont\" is included at the end, then the calculation will\n";
	cout << "continue from the last point reading other values from the .chisq file.\n";
	cout << "If \"read\" is included at the end, then the calculation will\n";
	cout << "check the corresponding .chisq file for previous calculations at each meshpoint\n";
	cout << "so as not to repeat a step. This cannot be declared together with \"cont\".\n";
	cout << "If \"-dN\" is included at the end, where N is the index of the diagonal matrix\n";
	cout << "element other than the default 2, then correct lines of the .bst/.bst.lit\n";
	cout << "files will be read/written.\n";

	return;
	
}

int main( int argc, char* argv[] ) {
	
	// If the number of arguments are wrong, exit with usage
	if( argc < 2 || argc > 14 ) {
		
		PrintUsage(argv[0]);
		return 0;
	
	}
	
	// Get/Set arguments
	stringstream arg ( stringstream::in | stringstream::out );
	stringstream tmp ( stringstream::in | stringstream::out );

	string in_proj, in_targ;
	int Ndata_proj = 3;
	int Ndata_targ = 5;
	double low_tme = 0.1;
	double upp_tme = 2.5;
	int Nsteps_tme = 51;
	double low_dme = 0.0;
	double upp_dme = 0.0;
	int Nsteps_dme = 1;
	bool cont = false;
	bool read = false;
	int tme_index = 1; // default, index 1
	int dme_index = 2; // default, index 2
	
	if( argc >= 2 ) {
		arg.str("");
		arg.clear();
		arg << argv[1];
		arg >> in_proj;
	}
	if( argc >= 3 ) {
		arg.str("");
		arg.clear();
		arg << argv[2];
		arg >> in_targ;
	}
	if( argc >= 4 ) {
		arg.str("");
		arg.clear();
		arg << argv[3];
		arg >> Ndata_proj;
	}
	if( argc >= 5 ) {
		arg.str("");
		arg.clear();
		arg << argv[4];
		arg >> Ndata_targ;
	}
	if( argc >= 6 ) {
		arg.str("");
		arg.clear();
		arg << argv[5];
		arg >> low_tme;
	}
	if( argc >= 7 ) {
		arg.str("");
		arg.clear();
		arg << argv[6];
		arg >> upp_tme;
	}
	if( argc >= 8 ) {
		arg.str("");
		arg.clear();
		arg << argv[7];
		arg >> Nsteps_tme;
	}
	if( argc >= 9 ) {
		arg.str("");
		arg.clear();
		arg << argv[8];
		arg >> low_dme;
	}
	if( argc >= 10 ) {
		arg.str("");
		arg.clear();
		arg << argv[9];
		arg >> upp_dme;
	}
	if( argc >= 11 ) {
		arg.str("");
		arg.clear();
		arg << argv[10];
		arg >> Nsteps_dme;
	}
	if( argc >= 12 ) {
		for ( int i = 11; i <= argc; i++ ) {
			arg.str("");
			arg.clear();
			arg << argv[i];
			if ( arg.str() == "cont" ) cont = true;
			else if ( arg.str() == "read" ) read = true;
			else if ( arg.str().substr(0,2) == "-t" ) {
				tmp.str("");
				tmp.clear();
				tmp << arg.str().substr(2,1);
				tmp >> tme_index;
				cout << "Transitional matrix element, index " << tme_index << endl;
			}
			else if ( arg.str().substr(0,2) == "-d" ) {
				tmp.str("");
				tmp.clear();
				tmp << arg.str().substr(2,1);
				tmp >> dme_index;
				cout << "Diagonal matrix element, index " << dme_index << endl;
			}
		}
	}
	
	if( read && cont ){
		cout << "Cannot declare \"read\" and \"cont\"... Pick one.\n";
		return 0;
	}
		
	// Open output text and root files
	//  if continuing "cont" read output file first
	//  if not, copy old files to ...old 
	string outname = in_proj.substr( 0, in_proj.find_last_of(".") );
	string textname = outname + ".chisq";
	string rsltname = outname + ".rslt";
	string rootname = outname + ".root";
	ofstream out, rslt;
	ifstream old;
	string cmd;
	if ( cont || read ) old.open( textname.c_str(), ios::in );	
	else out.open( textname.c_str(), ios::out );
	
	cmd = "cp " + textname + " " + textname + ".old";
	if( system(NULL) ) system( cmd.c_str() );
	cmd = "cp " + rootname + " " + rootname + ".old";
	if( system(NULL) ) system( cmd.c_str() );
	cmd = "cp " + rsltname + " " + rsltname + ".old";
	if( system(NULL) ) system( cmd.c_str() );

	rslt.open( rsltname.c_str(), ios::out );	
	
	TFile *root = new TFile( rootname.c_str(), "RECREATE" );
		
	// Initiate variables
	int intiflag;
	int metest;
	bool do_calc = true;
	double dme, tme, dme_prv, tme_prv;
	double chisq = 999., chisq_proj = 999., chisq_targ = 999.;
	double stepSize_dme = ( upp_dme - low_dme ) / (double)(Nsteps_dme-1);
	double stepSize_tme = ( upp_tme - low_tme ) / (double)(Nsteps_tme-1);
	if( Nsteps_dme == 1 ) stepSize_dme = 0;
	if( Nsteps_tme == 1 ) stepSize_tme = 0;
	
	vector<double> result_vector;	
	int index;

	// If continuing or reading old values, get last calculated values
	if ( cont || read ) {
		if ( old.is_open() ) {

			old >> dme_prv >> tme_prv >> chisq_proj >> chisq_targ >> chisq;
			while ( !old.eof() ) {

				result_vector.push_back( (double)dme_prv );
				result_vector.push_back( (double)tme_prv );
				result_vector.push_back( (double)chisq_proj );
				result_vector.push_back( (double)chisq_targ );
				result_vector.push_back( (double)chisq );

				old >> dme_prv >> tme_prv >> chisq_proj >> chisq_targ >> chisq;
			
			}
			
			chisq = chisq_proj = chisq_targ = 999.;
	
			cout << "\nRead array of " << int(result_vector.size()/5);
			cout << " results from previous calculation...\n";
							
			// Reopen file for writing
			old.close();
			out.open( textname.c_str(), ios::out );
			
			if ( cont ) {			
				cout << "\nContinuing from a previous state...\n";
				cout << "Previous DME = " << dme_prv << endl;
				cout << "Previous TME = " << tme_prv << endl << endl;
			}
			
		}
		else {
		
			cout << "Cannot open " << textname.c_str() << " in order to resume\n";
			cont = false;
		
		}
	}
	

	// A 2-dimensional chisq graph in root plus 1sigma cut
	TGraph2D *gChisq = new TGraph2D( Nsteps_dme*Nsteps_tme );
	TGraph2D *gChisq_proj = new TGraph2D();
	TGraph2D *gChisq_targ = new TGraph2D();
	TGraph2D *gChisq_1sigma = new TGraph2D();
	TGraph2D *gChisq_1sigma_rotorlim = new TGraph2D();
	TGraph2D *gChisq_1sigma_2rotorlim = new TGraph2D();
	gChisq->SetName("gChisq");
	gChisq_proj->SetName("gChisq_proj");
	gChisq_targ->SetName("gChisq_targ");
	gChisq_1sigma->SetName("gChisq_1sigma");
	gChisq->SetTitle("#chi^{2} surface plot (N_{p}#chi^{2}_{p} + N_{t}#chi^{2}_{t});<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	gChisq_proj->SetTitle("#chi^{2} surface plot for projectile;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	gChisq_targ->SetTitle("#chi^{2} surface plot for target;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	gChisq_1sigma->SetTitle("#chi^{2}+1 cut;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	gChisq_1sigma_rotorlim->SetTitle("#chi^{2}+1 cut plus rigid rotor limits;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	gChisq_1sigma_2rotorlim->SetTitle("#chi^{2}+1 cut plus twice the rigid rotor limits;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}");
	if( Nsteps_dme > 4 && Nsteps_dme < 999 ) {
		gChisq->SetNpx(Nsteps_dme-1);
		gChisq_1sigma->SetNpx(Nsteps_dme-1);
		gChisq_1sigma_rotorlim->SetNpx(Nsteps_dme-1);
		gChisq_1sigma_2rotorlim->SetNpx(Nsteps_dme-1);
	}
	if( Nsteps_tme > 4 && Nsteps_tme < 999 ) {
		gChisq->SetNpy(Nsteps_tme);
		gChisq_1sigma->SetNpy(Nsteps_tme);
		gChisq_1sigma_rotorlim->SetNpy(Nsteps_tme);
		gChisq_1sigma_2rotorlim->SetNpy(Nsteps_tme);
	}
	
	// 1-dimensional chisq graphs for each dme
	string gName, gTitle, dme_str;
	TGraph *gChisqDME[(const int)Nsteps_dme];
	for( int id = 0; id < Nsteps_dme; id++ ){
		dme_str = convertFloat( (float)(low_dme + id*stepSize_dme) );
		gName = "gChisq_" + convertInt(id);
		gTitle = "#chi^{2} surface plot for DME = " + dme_str;
		gTitle += " eb;<0^{+}||E2||2^{+}> [eb];#chi^{2}";
		gChisqDME[id] = new TGraph(Nsteps_tme);
		gChisqDME[id]->SetTitle(gTitle.c_str());
	}
	
	// 2-D histograms for projectile and target
	TH2D *hChisq = new TH2D("hChisq",
		"#chi^{2} surface plot (N_{p}#chi^{2}_{p} + N_{t}#chi^{2}_{t});<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);
	TH2D *hChisq_proj = new TH2D("hChisq_proj",
		"#chi^{2} surface plot for projectile N_{p}#chi^{2}_{p};<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);
	TH2D *hChisq_targ = new TH2D("hChisq_targ",
		"#chi^{2} surface plot for target N_{t}#chi^{2}_{t};<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);

		// Get chisq values and write to file
	cout << "Ndata projectile: " << Ndata_proj << endl;
	cout << "          target: " << Ndata_targ << endl;
	cout << "\n\t\t  Chi-squared value\n";
	cout << "DME\tTME\tProj\tTarg\tTotal\n";
	for ( int i=0; i<Nsteps_dme; i++ ) {
	
		dme = low_dme + i*stepSize_dme;

		for ( int j=0; j<Nsteps_tme; j++ ) {
		
			tme = low_tme + j*stepSize_tme;
			
			if ( read ) {
			
				index = LookUpOldChisq( result_vector, dme, tme );
				if ( index < 0 ) do_calc = true;
				else {
					chisq_proj = result_vector.at( 5*index+2 );
					chisq_targ = result_vector.at( 5*index+3 );
					chisq = result_vector.at( 5*index+4 );		
					do_calc = false;
				}
			
			} 
			
			else if ( cont ) {
			
				// Read previous values
				dme_prv = result_vector.at( 5*i*Nsteps_tme+0 );
				tme_prv = result_vector.at( 5*i*Nsteps_tme+1 );
				chisq_proj = result_vector.at( 5*i*Nsteps_tme+2 );
				chisq_targ = result_vector.at( 5*i*Nsteps_tme+3 );
				chisq = result_vector.at( 5*i*Nsteps_tme+4 );		
				
				// Check for consistency
				if ( TMath::Abs(dme_prv-dme) < 1E-8 && TMath::Abs(tme_prv-tme) < 1E-8 ) {
				
					do_calc = false;
					
				} else {
					
					cout << "\nPrevious matrix elements don't match\n";
					cout << "Make sure you're re-running in the exact same state,\n";
					cout << "or else you should use the \"read\" option.\n";
					do_calc = true;
				
				}
			
			} 
			
			else do_calc = true;
			
			cout << dme << "\t" << tme << "\t";

			if ( do_calc == true ) {
			
				// Write matrix elements
				metest = WriteMatrixElementsToFile( in_proj, in_targ, tme, dme, tme_index, dme_index );
				if ( metest == 1 ) {
					cout << "Couldn't write projectile matrix elements to file\n";
					continue;
				}
//				else if ( metest == 2 ) cerr << "Couldn't copy literature target matrix elements to file\n";

				// Integration step
				intiflag = IntegrateProjectile( in_proj );
				
				// Run gosia and return chisq values
				if ( GetChiSq( in_proj, in_targ, chisq_proj, chisq_targ ) == 0 ) {
					cout << "Unable to run gosia2\n";
					continue;
				}
				chisq_proj *= Ndata_proj;
				chisq_targ *= Ndata_targ;
				chisq = chisq_proj + chisq_targ;
			
			}
	
			cout << chisq_proj << "\t" << chisq_targ << "\t" << chisq << endl;
			out << dme << "\t" << tme << "\t" << chisq_proj << "\t";
			out << chisq_targ << "\t" << chisq << endl;

			gChisq->SetPoint( i*Nsteps_tme + j, dme, tme, chisq );
			hChisq->SetBinContent( i+1, j+1, chisq );
			gChisq_proj->SetPoint( i*Nsteps_tme + j, dme, tme, chisq_proj );
			gChisq_targ->SetPoint( i*Nsteps_tme + j, dme, tme, chisq_targ );
			hChisq_proj->SetBinContent( i+1, j+1, chisq_proj );
			hChisq_targ->SetBinContent( i+1, j+1, chisq_targ );
			gChisqDME[i]->SetPoint( j, tme, chisq );
				
		}

	}
	
	// Make 1sigma cut (chisq_min+1) and write values
	double chisq_min = gChisq->GetZmin();
	int ix, iy, iz;
	TH2D *hChisq_1sigma = new TH2D("hChisq_1sigma",
		"#chi^{2}+1 cut;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);
	TH2D *hChisq_1sigma_rotorlim = new TH2D("hChisq_1sigma_rotorlim",
		"#chi^{2}+1 cut plus rigid rotor limits;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);
	TH2D *hChisq_1sigma_2rotorlim = new TH2D("hChisq_1sigma_2rotorlim",
		"#chi^{2}+1 cut plus twice the rigid rotor limits;<2^{+}||E2||2^{+}> [eb];<0^{+}||E2||2^{+}> [eb];#chi^{2}",
		Nsteps_dme, low_dme-0.5*stepSize_dme, upp_dme+0.5*stepSize_dme,
		Nsteps_tme, low_tme-0.5*stepSize_tme, upp_tme+0.5*stepSize_tme);

	double tme_tmp, dme_tmp, chisq_tmp;
	int ctr1 = 0, ctr2 = 0, ctr3 = 0;
	for( int p = 0; p < Nsteps_dme; p++ ) {

		dme_tmp = low_dme + p*stepSize_dme;

		for( int q = 0; q < Nsteps_tme; q++ ) {

			tme_tmp = low_tme + q*stepSize_tme;
			chisq_tmp = hChisq->GetBinContent( p+1, q+1 );

			if( chisq_tmp <= chisq_min+1 ) {

				gChisq_1sigma->SetPoint( ctr1, dme_tmp, tme_tmp, chisq_tmp );
				hChisq_1sigma->SetBinContent( p+1, q+1, chisq_tmp );
				ctr1++;
		
				if( TMath::Abs(dme_tmp) <= tme_tmp*1.19523 ) {

					gChisq_1sigma_rotorlim->SetPoint(ctr2,dme_tmp,tme_tmp,chisq_tmp);
					hChisq_1sigma_rotorlim->SetBinContent(p+1,q+1,chisq_tmp);
					ctr2++;

				}
			
				if( TMath::Abs(dme_tmp) <= tme_tmp*2*1.19523 ) {

					gChisq_1sigma_2rotorlim->SetPoint(ctr3,dme_tmp,tme_tmp,chisq_tmp);
					hChisq_1sigma_2rotorlim->SetBinContent(p+1,q+1,chisq_tmp);
					ctr3++;

				}

			}

		}	

	}
	
	hChisq->GetMinimumBin(ix,iy,iz);
	double dme_min = hChisq->GetXaxis()->GetBinCenter(ix);
	double tme_min = hChisq->GetYaxis()->GetBinCenter(iy);

	double tme_err_low = 0.5*stepSize_tme, tme_err_upp = 0.5*stepSize_tme;
	double dme_err_low = 0.5*stepSize_dme, dme_err_upp = 0.5*stepSize_dme;

	tme_err_low += TMath::Abs( tme_min - gChisq_1sigma->GetYmin() );
	tme_err_upp += TMath::Abs( tme_min - gChisq_1sigma->GetYmax() );
	dme_err_low += TMath::Abs( dme_min - gChisq_1sigma->GetXmin() );
	dme_err_upp += TMath::Abs( dme_min - gChisq_1sigma->GetXmax() );

//	TH2D *hChisq_1sigma = (TH2D*)gChisq_1sigma->GetHistogram();

	cout << "\nChisq minimum found at " << chisq_min << endl;
	cout << "<0+1||E2||2+1> = " << tme_min << "(-" << tme_err_low;
	cout << "; +" << tme_err_upp << ")" << endl;
	cout << "<2+1||E2||2+1> = " << dme_min << "(-" << dme_err_low;
	cout << "; +" << dme_err_upp << ")\n" << endl;
	
	rslt << "<0+1||E2||2+1> = " << tme_min << "\t-" << tme_err_low << "\t+" << tme_err_upp << endl;
	rslt << "<2+1||E2||2+1> = " << dme_min << "\t-" << dme_err_low << "\t+" << dme_err_upp << endl;
	rslt << "Chisq minimum = " << chisq_min << endl;
	rslt << "Ndata projectile: " << Ndata_proj << endl;
	rslt << "          target: " << Ndata_targ << endl;
	
	if( intiflag == 1 ) cout << "Integration performed at each step\n";
	else cout << "Integration performed with starting parameters only\n\n";

	// Write files and close
	root->cd();
	gChisq->Write("gChisq");
	gChisq_proj->Write("gChisq_proj");
	gChisq_targ->Write("gChisq_targ");
	gChisq_1sigma->Write("gChisq_1sigma");
	gChisq_1sigma_rotorlim->Write("gChisq_1sigma_rotorlim");
	gChisq_1sigma_2rotorlim->Write("gChisq_1sigma_2rotorlim");
	hChisq->Write("hChisq");
	hChisq_proj->Write("hChisq_proj");
	hChisq_targ->Write("hChisq_targ");
	hChisq_1sigma->Write("hChisq_1sigma");
	hChisq_1sigma_rotorlim->Write("hChisq_1sigma_rotorlim");
	hChisq_1sigma_2rotorlim->Write("hChisq_1sigma_2rotorlim");
	for( int id = 0; id < Nsteps_dme; id++ ) {
		gName = "gChisq_DME_" + convertInt(id);
		gChisqDME[id]->Write(gName.c_str());
	}
	root->Close();
	out.close();
	rslt.close();

	cout << "I wrote the data to...\n\tROOT file: " << rootname << endl;
	cout << "\tTEXT file: " << textname << endl;
	cout << "I wrote the results to " << rsltname << endl;

	return 0;
	
}
