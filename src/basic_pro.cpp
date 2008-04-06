/***************************************************************************
                          basic_pro.cpp  -  basic GDL library procedures
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002 by Marc Schellens
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"

#include <string>
#include <fstream>
#include <memory>
#include <set>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __APPLE__
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
#include <unistd.h>
#endif

#ifdef HAVE_LIBWXWIDGETS
#include <wx/wx.h>
#endif

#include "dinterpreter.hpp"
#include "datatypes.hpp"
#include "envt.hpp"
#include "dpro.hpp"
#include "io.hpp"
#include "basic_pro.hpp"

namespace lib {
 
  using namespace std;

  // display help for one variable or one structure tag
  void help_item( ostream& os,
		  BaseGDL* par, DString parString, bool doIndentation)
  {
    if( doIndentation) os << "   ";

    // Name display
    os.width(16);
    os << left << parString;
    if( parString.length() >= 16)
      {
        os << endl;
        os.width(doIndentation? 19:16);
        os << "";
      }

    // Type display
    if( !par)
      {
        os << "UNDEFINED = <Undefined>" << endl;
        return;
      }
    os.width(10);
    os << par->TypeStr() << right;

    if( !doIndentation) os << "= ";

    // Data display
    if( par->Type() == STRUCT)
      {
        DStructGDL* s = static_cast<DStructGDL*>( par);
        os << "-> ";
        os << (s->Desc()->IsUnnamed()? "<Anonymous>" : s->Desc()->Name());
	os << " ";
      }
    else if( par->Dim( 0) == 0)
      {
        if (par->Type() == STRING)
	  {
            // trim string larger than 45 characters
            DString dataString = (*static_cast<DStringGDL*>(par))[0];
            os << "'" << StrMid( dataString,0,45,0) << "'";
	    if( dataString.length() > 45) os << "...";
	  }
	else
	  {
            par->ToStream( os);
	  }
      }

    // Dimension display
    if( par->Dim( 0) != 0) os << par->Dim();

    // End of line
    os << endl;
  }


  DStringGDL* recall_commands_internal()//EnvT* e)
  {
    //int status=0;
    DStringGDL* retVal;
    retVal = new DStringGDL(1, BaseGDL::NOZERO);
    (*retVal)[ 0] ="";

#if defined(HAVE_LIBREADLINE) && !defined(__APPLE__)
    //status=1;
    // http://cnswww.cns.cwru.edu/php/chet/readline/history.html#IDX14
    HIST_ENTRY **the_list;
    //    cout << "history_length" << history_length << endl;
    the_list = history_list ();

    if (the_list) {
      retVal = new DStringGDL( history_length-1, BaseGDL::NOZERO);
      for (SizeT i = 0; i<history_length-1 ; i++)
	(*retVal)[ i] = the_list[i]->line;
    }
#else
//     if (status == 0) {
      Message("RECALL_COMMANDS: nothing done, because compiled without READLINE");
//     }
#endif
    return retVal;
  }
 
  BaseGDL* recall_commands( EnvT* e)
  {
    return recall_commands_internal();
  }

  void help( EnvT* e)
  {
    bool kw = false;

    static int callsKWIx = e->KeywordIx("CALLS");
    bool callsKW = e->KeywordPresent( callsKWIx);
    if( callsKW)
      {
	EnvStackT& cS = e->Interpreter()->CallStack();

	SizeT level = cS.size();

	assert( level > 1); // HELP, $MAIN$

	DStringGDL* retVal = new DStringGDL( dimension( level-1), BaseGDL::NOZERO);
	SizeT rIx = 0;
	for( EnvStackT::reverse_iterator r = cS.rbegin()+1; r != cS.rend(); ++r)
	  {
	    EnvBaseT* actEnv = *r;
	    assert( actEnv != NULL);

	    DString actString = actEnv->GetProName();
	    DSubUD* actSub = dynamic_cast<DSubUD*>(actEnv->GetPro());
	    if( (r+1) != cS.rend() && actSub != NULL)
	      {
		actString += " <"+actSub->GetFilename() + "(";
		if( (*(r-1))->CallingNode() != NULL)
		  actString += i2s( (*(r-1))->CallingNode()->getLine(), 4);
		else
		  actString += "   ?";
		actString += ")>";
	      }
	    
	    (*retVal)[ rIx++] = actString;
	  }

	e->SetKW( callsKWIx, retVal);
	return;
      }

    if( e->KeywordSet( "INFO"))
      {
	kw = true;

	cout << "Homepage: http://gnudatalanguage.sf.net" << endl;
	cout << "HELP,/LIB for a list of all internal library "
	  "functions/procedures." << endl;
	cout << "Additional subroutines are written in GDL language, "
	  "look for *.pro files." << endl;
	cout << endl;
      }

    if( e->KeywordSet( "LIB"))
      {
	kw = true;

	deque<DString> subList;
	SizeT nPro = libProList.size();
	cout << "Library procedures (" << nPro <<"):" << endl;
	for( SizeT i = 0; i<nPro; ++i)
	  subList.push_back(libProList[ i]->ToString());

	sort( subList.begin(), subList.end());

	for( SizeT i = 0; i<nPro; ++i)
	  cout << subList[ i] << endl;

	subList.clear();

	SizeT nFun = libFunList.size();
	cout << "Library functions (" << nFun <<"):" << endl;
	for( SizeT i = 0; i<nFun; ++i)
	  subList.push_back(libFunList[ i]->ToString());

	sort( subList.begin(), subList.end());

	for( SizeT i = 0; i<nFun; ++i)
	  cout << subList[ i] << endl;
      }

    bool isKWSetStructures = e->KeywordSet( "STRUCTURES");
    if( isKWSetStructures) kw = true;

    bool isKWSetProcedures = e->KeywordSet( "PROCEDURES");
    bool isKWSetFunctions  = e->KeywordSet( "FUNCTIONS");
   
    if (isKWSetStructures && (isKWSetProcedures || isKWSetFunctions))
      e->Throw( "Conflicting keywords.");	
    
    bool isKWSetRecall  = e->KeywordSet( "RECALL_COMMANDS");
    if (isKWSetRecall && (isKWSetProcedures || isKWSetFunctions))
      e->Throw( "Conflicting keywords.");
    
    // using this way, we don't need to manage HAVE_READLINE at this level ...
    if (isKWSetRecall) {
      DStringGDL *previous_commands;
      previous_commands=recall_commands_internal();
      SizeT nEl2 = previous_commands->N_Elements();
      cout << "Recall buffer length: " << nEl2 << endl;
	for( SizeT i=0; i<nEl2; ++i)
	  cout << i+1 << "  " <<(*previous_commands)[i] << endl;
      return;
    }

    SizeT nParam=e->NParam();
    std::ostringstream ostr;

    // Compiled Procedures & Functions
    DLong np = proList.size() + 1;
    DLong nf = funList.size();
    deque<DString> pList;
    deque<DString> fList;

    // If OUTPUT keyword set then set up output string array (outputKW)
    BaseGDL** outputKW = NULL;
    static int outputIx = e->KeywordIx( "OUTPUT");
    if( e->KeywordPresent( outputIx)) {
      SizeT nlines = 0;
      if (isKWSetProcedures) {
	nlines = np + 1;
      } else if (isKWSetFunctions) {
	nlines = nf + 1;
      } else {
	// Determine the number of entries in the output array
	for( SizeT i=0; i<nParam; i++)
	  {
	    BaseGDL*& par=e->GetPar( i);
	    DString parString = e->Caller()->GetString( par);
	    if( !par || !isKWSetStructures || par->Type() != STRUCT) {
	      nlines++;
	    } else {
	      DStructGDL* s = static_cast<DStructGDL*>( par);
	      SizeT nTags = s->Desc()->NTags();
	      nlines++;
	      nlines += nTags;
	    }
	  }

	// Add space for compiled procedures & functions
	if (nParam == 0) {
	  // list all variables of caller
	  EnvBaseT* caller = e->Caller();
	  SizeT nEnv = caller->EnvSize();
	  nlines = nEnv + 5;
	}
      }

      // Setup output return variable
      outputKW = &e->GetKW( outputIx);
      delete (*outputKW);
      dimension dim(&nlines, (size_t) 1);
      *outputKW = new DStringGDL(dim, BaseGDL::NOZERO);
    }

    // switch to dec output (might be changed from formatted output)
    if( outputKW == NULL)
      cout << dec;

    static int routinesKWIx = e->KeywordIx("ROUTINES");
    static int briefKWIx = e->KeywordIx("BRIEF");
    bool routinesKW = e->KeywordSet( routinesKWIx);
    bool briefKW = e->KeywordSet( briefKWIx);
    SizeT nOut = 0;
    
    if (nParam == 0 || isKWSetFunctions || isKWSetProcedures) {

      if (nParam == 0 && !isKWSetFunctions && !isKWSetProcedures) {
	// Tell where we are
	DSubUD* pro = static_cast<DSubUD*>( e->Caller()->GetPro());
	if (outputKW == NULL) {
	  cout << "% At " << pro->ObjectName() << endl;
	} else {
	  ostr << "% At " << pro->ObjectName();
	  (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	  ostr.str("");
	}
      }

      // Get list of user procedures
      pList.push_back("$MAIN$");
      for( ProListT::iterator i=proList.begin(); i != proList.end(); i++)
	pList.push_back((*i)->ObjectName());
      sort( pList.begin(), pList.end());

      // Get list of user functions
      for( FunListT::iterator i=funList.begin(); i != funList.end(); i++)
	fList.push_back((*i)->ObjectName());
      sort( fList.begin(), fList.end());

      // PROCEDURES keyword
      if (isKWSetProcedures) {
	if (outputKW == NULL) {
	    cout << "Compiled Procedures:" << endl;
	} else {
	  ostr << "Compiled Procedures:";
	  (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	  ostr.str("");
	}

	// Loop through procedures
	for( SizeT i=0; i<np; i++) {
	  // Add $MAIN$
	  if (i == 0) {
	    if (outputKW == NULL) {
	      cout << "$MAIN$";
	    } else {
	      ostr << "$MAIN$";
	      (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	      ostr.str("");
	    }
	  }

	  // Find DPro pointer for pList[i]
	  ProListT::iterator p=std::find_if(proList.begin(),proList.end(),
					    Is_eq<DPro>(pList[i]));
	  if( p != proList.end()) {
	    DPro *pro = *p;
	    int nPar = pro->NPar();
	    int nKey = pro->NKey();

	    // Loop through parameters
	    if (outputKW == NULL) {
	      cout << setw(25) << left << pro->ObjectName() << setw(0);
	      for( SizeT j=0; j<nPar; j++)
		cout << StrLowCase(pro->GetVarName(nKey+j)) << " ";
	    } else {
	      ostr << setw(25) << left << pro->ObjectName() << setw(0);
	      for( SizeT j=0; j<nPar; j++)
		ostr << StrLowCase(pro->GetVarName(nKey+j)) << " ";
	      (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	      ostr.str("");
	    }
	  }
	  if (outputKW == NULL) cout << endl;
	}
	// FUNCTIONS keyword
      } else if (isKWSetFunctions) {

	if (outputKW == NULL) {
	  cout << "Compiled Functions:" << endl;
	} else {
	  ostr << "Compiled Functions:";
	  (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	  ostr.str("");
	}

	// Loop through functions
	for( SizeT i=0; i<nf; i++) {

	  // Find DFun pointer for fList[i]
	  FunListT::iterator p=std::find_if(funList.begin(),funList.end(),
					    Is_eq<DFun>(fList[i]));
	  if( p != funList.end()) {
	    DFun *pro = *p;
	    int nPar = pro->NPar();
	    int nKey = pro->NKey();

	    // Loop through parameters
	    if (outputKW == NULL) {
	      cout << setw(25) << left << pro->ObjectName() << setw(0);
	      for( SizeT j=0; j<nPar; j++)
		cout << StrLowCase(pro->GetVarName(nKey+j)) << " ";
	    } else {
	      ostr << setw(25) << left << pro->ObjectName() << setw(0);
	      for( SizeT j=0; j<nPar; j++)
		ostr << StrLowCase(pro->GetVarName(nKey+j)) << " ";
	      (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	      ostr.str("");
	    }
	  }
	  if (outputKW == NULL) cout << endl;
	}
      }
      if( isKWSetProcedures) return;
      if( isKWSetFunctions)  return;
    }

    for( SizeT i=0; i<nParam; i++)
      {
	BaseGDL*& par=e->GetPar( i);
	DString parString = e->Caller()->GetString( par);
	// NON-STRUCTURES
	if( !par || !isKWSetStructures || par->Type() != STRUCT)
          {
	    // If no OUTPUT keyword send to stdout
	    if (outputKW == NULL) {
	      help_item( cout, par, parString, false);
	    } else {
	      // else send to string stream & store in outputKW (remove CR)
	      help_item( ostr, par, parString, false);
	      (*(DStringGDL *) *outputKW)[nOut++] = 
		ostr.rdbuf()->str().erase(ostr.rdbuf()->str().length()-1,1); 
	      ostr.str("");
	    }
          }
        else
	  {
	    // STRUCTURES
            DStructGDL* s = static_cast<DStructGDL*>( par);
	    SizeT nTags = s->Desc()->NTags();
	    if (outputKW == NULL) {
	      cout << "** Structure ";
	      cout << (s->Desc()->IsUnnamed() ? "<Anonymous> " : 
		       s->Desc()->Name());
	      cout << ", " << nTags << " tags:" << endl;
	      for (SizeT t=0; t < nTags; ++t)
		{    
		  DString tagString = s->Desc()->TagName(t);
		  help_item( cout, s->GetTag(t), tagString, true);
		}
	    } else {
	      // OUTPUT KEYWORD SET
	      ostr << "** Structure ";
	      ostr << (s->Desc()->IsUnnamed() ? "<Anonymous> " : 
		       s->Desc()->Name());
	      ostr << ", " << nTags << " tags:";
	      (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	      ostr.str("");

	      for (SizeT t=0; t < nTags; ++t)
		{    
		  DString tagString = s->Desc()->TagName(t);
		  help_item( ostr, s->GetTag(t), tagString, true);
		  (*(DStringGDL *) *outputKW)[nOut++] = 
		    ostr.rdbuf()->str().erase(ostr.rdbuf()->str().
					      length()-1,1);
		  ostr.str("");
		}
	    }
	  }
      }
    if( routinesKW || briefKW) kw = true;

    if( nParam == 0 && !kw)
      {
	routinesKW = true;
	briefKW = true;

	// list all variables of caller
	EnvBaseT* caller = e->Caller();

	SizeT nEnv = caller->EnvSize();

	set<string> helpStr;  // "Sorted List" 
	for ( int i = 0; i < nEnv; ++i ) 
	  {
	    BaseGDL*& par=caller->GetKW( i);
	    if( par == NULL) 
	      continue;

	    DString parString = caller->GetString( par);
	    
	    stringstream ss;
	    help_item( ss, par, parString, false);
	    
	    helpStr.insert( ss.str() );
	  }

	  if (outputKW == NULL) {
	    copy( helpStr.begin(), helpStr.end(),
		  ostream_iterator<string>( cout) );
	  }


	// Display compiled procedures & functions
	if (!isKWSetProcedures && !isKWSetFunctions) {
	  // StdOut
	  if (outputKW == NULL) {
	    cout << "Compiled Procedures:" << endl;
	    for( SizeT i=0; i<np; i++) cout << pList[i] << " ";
	    cout << endl << endl;

	    cout << "Compiled Functions:" << endl;
	    for( SizeT i=0; i<nf; i++) cout << fList[i] << " ";
	    cout << endl;
	  } else {
	    // Keyword Output

	    // Output variables
	    set<string>::iterator it = helpStr.begin(); 
	    while(it != helpStr.end()) { 
	      ostr << *it;

	      (*(DStringGDL *) *outputKW)[nOut++] = 
		ostr.rdbuf()->str().erase(ostr.rdbuf()->str().length()-1,1); 

	      ++it;
	      ostr.str("");
	    } 

	    // Output procedures & functions
	    ostr << "Compiled Procedures:";
	    (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	    ostr.str("");

	    for( SizeT i=0; i<np; i++) ostr << pList[i] << " ";
	    (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	    ostr.str("");

	    ostr << "Compiled Functions:";
	    (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	    ostr.str("");

	    for( SizeT i=0; i<nf; i++) ostr << fList[i] << " ";
	    (*(DStringGDL *) *outputKW)[nOut++] = ostr.rdbuf()->str();
	    ostr.str("");
	  }

	}
    }

// 	stringstream strS;
// 	for( SizeT i=0; i<nEnv; ++i)
// 	  {
// 	    BaseGDL*& par=caller->GetKW( i);
// 	    if( par == NULL) 
// 	      continue;
	    
// 	    DString parString = caller->GetString( par);

// 	    help_item(  &strS, par, parString, false);
// 	  }

// 	deque<DString> toSort;
// 	for( SizeT i=0; i<nEnv; ++i)
// 	  {
// 	    char buf[ 256];
// 	    strS.getline( buf, 256);
// 	    toSort.push_back( buf);
// 	  }
// 	sort( toSort.begin(), toSort.end());
// 	for( SizeT i=0; i<nEnv; ++i)
// 	  {
// 	    cout << toSort[ i] << endl;
// 	  }
  }

  void exitgdl( EnvT* e)
  {

#ifdef HAVE_LIBWXWIDGETS
    wxTheApp->OnExit(); // Defined in GDLApp::OnExit() in gdlwidget.cpp
    wxUninitialize();
#endif

#ifdef HAVE_LIBREADLINE

    // we manage the ASCII "history" file (located in ~/.gdl/)
    // we do not manage NOW the number of lines we save,
    // this should be limited by "history/readline" itself
    
    // Create eventually the ".gdl" path in user $HOME
    int result, debug=0;
    char *homeDir = getenv( "HOME");
    string pathToGDL_history;
    pathToGDL_history=homeDir;
    AppendIfNeeded(pathToGDL_history, "/");
    pathToGDL_history=pathToGDL_history+".gdl";
    // Create eventially the ".gdl" path in Home
    result=mkdir(pathToGDL_history.c_str(), 0700);
    if (debug)
      { if (result == 0) { cout << "Creation of ~/.gdl PATH "<< endl;}
      else {cout << "~/.gdl PATH was still here "<< endl;}
      }
    
    // (over)write the history file in ~/.gdl PATH
  
    string history_filename;
    AppendIfNeeded(pathToGDL_history, "/");
    history_filename=pathToGDL_history+"history";
    if (debug) cout << "History file name: " <<history_filename << endl;
    result=write_history(history_filename.c_str());
    if (debug) 
      { if (result == 0) {cout<<"Successfull writing of ~/.gdl/history"<<endl;}
      else {cout <<"Fail to write ~/.gdl/history"<<endl;}
      }

#endif

    BaseGDL* status=e->GetKW( 1);
    if( status == NULL) exit( EXIT_SUCCESS);
    
    if( !status->Scalar())
      e->Throw( "Expression must be a scalar in this context: "+
		e->GetString( status));

    DLongGDL* statusL=static_cast<DLongGDL*>(status->Convert2( LONG, 
							       BaseGDL::COPY));
    
    DLong exit_status;
    statusL->Scalar( exit_status);
    exit( exit_status);
  }

  void heap_gc( EnvT* e)
  {
    static SizeT objIx = e->KeywordIx( "OBJ");
    static SizeT ptrIx = e->KeywordIx( "PTR");
    static SizeT verboseIx = e->KeywordIx( "VERBOSE");
    bool doObj = e->KeywordSet( objIx);
    bool doPtr = e->KeywordSet( ptrIx);
    bool verbose =  e->KeywordSet( verboseIx);
    if( !doObj && !doPtr)
      doObj = doPtr = true;

    e->HeapGC( doPtr, doObj, verbose);
  }

  void ptr_free( EnvT* e)
  {
    SizeT nParam=e->NParam();
    for( SizeT i=0; i<nParam; i++)
      {
	DPtrGDL* par=dynamic_cast<DPtrGDL*>(e->GetPar( i));
	if( par != NULL) 
	  {
	    e->FreeHeap( par);
	  }
	else
	  e->Throw( "Pointer type required"
		    " in this context: "+e->GetParString(i));
      }
  }

  void obj_destroy( EnvT* e)
  {
    StackGuard<EnvStackT> guard( e->Interpreter()->CallStack());

    int nParam=e->NParam();
    if( nParam == 0) return;
    
    BaseGDL* p= e->GetParDefined( 0);

    DObjGDL* op= dynamic_cast<DObjGDL*>(p);
    if( op == NULL)
      e->Throw( "Parameter must be an object in"
		" this context: "+
		e->GetParString(0));

    SizeT nEl=op->N_Elements();
    for( SizeT i=0; i<nEl; i++)
      {
	DObj actID=(*op)[i];
	e->ObjCleanup( actID);
      }
  }
  
  void call_procedure( EnvT* e)
  {
    StackGuard<EnvStackT> guard( e->Interpreter()->CallStack());

    int nParam=e->NParam();
    if( nParam == 0)
      e->Throw( "No procedure specified.");
    
    DString callP;
    e->AssureScalarPar<DStringGDL>( 0, callP);

    // this is a procedure name -> convert to UPPERCASE
    callP = StrUpCase( callP);

    // first search library procedures
    int proIx=LibProIx( callP);
    if( proIx != -1)
      {
	e->PushNewEnv( libProList[ proIx], 1);
	
	// make the call
	EnvT* newEnv = static_cast<EnvT*>(e->Interpreter()->CallStack().back());
	static_cast<DLibPro*>(newEnv->GetPro())->Pro()(newEnv);
      }
    else
      {
	proIx = DInterpreter::GetProIx( callP);
	
	e->PushNewEnvUD( proList[ proIx], 1);
	
	// make the call
	EnvUDT* newEnv = static_cast<EnvUDT*>(e->Interpreter()->CallStack().back());
	e->Interpreter()->call_pro(static_cast<DSubUD*>(newEnv->GetPro())->
				   GetTree());
      }
  }

  void call_method_procedure( EnvT* e)
  {
    StackGuard<EnvStackT> guard( e->Interpreter()->CallStack());

    int nParam=e->NParam();
    if( nParam < 2)
      e->Throw( "Name and object reference must be specified.");
    
    DString callP;
    e->AssureScalarPar<DStringGDL>( 0, callP);

    // this is a procedure name -> convert to UPPERCASE
    callP = StrUpCase( callP);

    DStructGDL* oStruct = e->GetObjectPar( 1);
    
    DPro* method= oStruct->Desc()->GetPro( callP);

    if( method == NULL)
      e->Throw( "Method not found: "+callP);

    e->PushNewEnvUD( method, 2, &e->GetPar( 1));
    
    // the call
    e->Interpreter()->call_pro( method->GetTree());
  }

  void get_lun( EnvT* e)
  {
    int nParam=e->NParam( 1);
    
    // not using SetPar later gives a better error message
    e->AssureGlobalPar( 0);
    
    // here lun is the GDL lun, not the internal one
    DLong lun = GetLUN();

    if( lun == 0)
      e->Throw( "All available logical units are currently in use.");

    BaseGDL** retLun = &e->GetPar( 0);
    
    delete (*retLun); 
    //            if( *retLun != e->Caller()->Object()) delete (*retLun); 
    
    *retLun = new DLongGDL( lun);
    return;
  }

  // returns true if lun points to special unit
  // lun is GDL lun (-2..128)
  bool check_lun( EnvT* e, DLong lun)
  {
    if( lun < -2 || lun > maxLun)
      e->Throw( "File unit is not within allowed range: "+
		i2s(lun)+".");
    return (lun <= 0);
  }
  
  // TODO: handle ON_ERROR, ON_IOERROR, !ERROR_STATE.MSG
  void open_lun( EnvT* e, fstream::openmode mode)
  {
    int nParam=e->NParam( 2);
    
    if( e->KeywordSet( "GET_LUN")) get_lun( e);
    // par 0 contains now the LUN

    DLong lun;
    e->AssureLongScalarPar( 0, lun);

    bool stdLun = check_lun( e, lun);
    if( stdLun)
      e->Throw( "Unit already open. Unit: "+i2s( lun));
    
    DString name;
    // IDL allows here also arrays of length 1
    e->AssureScalarPar<DStringGDL>( 1, name); 

    // Change leading "~" to home directory
    if (name.substr(0,2) == "~/")
      name = getenv("HOME") + name.substr(1,name.size()-1);

    // endian
    bool swapEndian=false;
    static int swapIx = e->KeywordIx( "SWAP_ENDIAN");
    static int swapIfBigIx = e->KeywordIx( "SWAP_IF_BIG_ENDIAN");
    static int swapIfLittleIx = e->KeywordIx( "SWAP_IF_LITTLE_ENDIAN");
    if( e->KeywordSet( swapIx))
      swapEndian = true;
    else if( BigEndian())
      swapEndian = e->KeywordSet( swapIfBigIx);
    else
      swapEndian = e->KeywordSet( swapIfLittleIx);
    
    // compress
    bool compress=false;
    static int compressIx = e->KeywordIx( "COMPRESS");
    if( e->KeywordSet( compressIx))
      compress = true;

    // xdr
    static int xdrIx = e->KeywordIx( "XDR");
    bool xdr = e->KeywordSet( xdrIx);

    static int appendIx = e->KeywordIx( "APPEND");
    if( e->KeywordSet( appendIx)) mode |= fstream::ate;// fstream::app;

    static int f77Ix = e->KeywordIx( "F77_UNFORMATTED");
    bool f77 = e->KeywordSet( f77Ix);

    static int delIx = e->KeywordIx( "DELETE");
    bool deleteKey = e->KeywordSet( delIx);
    
    static int errorIx = e->KeywordIx( "ERROR");
    bool errorKeyword = e->KeywordPresent( errorIx);
    if( errorKeyword) e->AssureGlobalKW( errorIx);

    DLong width = defaultStreamWidth;
    static int widthIx = e->KeywordIx( "WIDTH");
    BaseGDL* widthKeyword = e->GetKW( widthIx);
    if( widthKeyword != NULL)
      {
	e->AssureLongScalarKW( widthIx, width);
      }

    // Assume variable-length VMS file initially
    fileUnits[ lun-1].PutVarLenVMS( true);

    try{
      fileUnits[ lun-1].Open( name, mode, swapEndian, deleteKey, 
			      xdr, width, f77, compress);
    } 
    catch( GDLException& ex) {
      DString errorMsg = ex.toString()+" Unit: "+i2s( lun)+
	", File: "+fileUnits[ lun-1].Name();
      
      if( !errorKeyword)
	throw GDLIOException( e->CallingNode(), e->GetPro()->ObjectName()+
			  ": "+errorMsg);
      
      BaseGDL** err = &e->GetKW( errorIx);
      
      delete (*err); 
//    if( *err != e->Caller()->Object()) delete (*err); 
      
      *err = new DLongGDL( 1);
      return;
    }

    if( errorKeyword)
      {
	BaseGDL** err = &e->GetKW( errorIx);
      
// 	if( *err != e->Caller()->Object()) delete (*err); 
	delete (*err); 
      
	*err = new DLongGDL( 0);
      }
  }
  
  void openr( EnvT* e)
  {
    open_lun( e, fstream::in);
  }

  void openw( EnvT* e)
  {
    static int appendKWIx = e->KeywordIx( "APPEND");
    if( e->KeywordSet( appendKWIx)) 
      {
	open_lun( e, fstream::in | fstream::out);
      } 
    else 
      {
	open_lun( e, fstream::in | fstream::out | fstream::trunc);
      }
  }

  void openu( EnvT* e)
  {
    open_lun( e, fstream::in | fstream::out);
  }
  
  void socket( EnvT* e)
  {
    int nParam=e->NParam( 3);
    
    if( e->KeywordSet( "GET_LUN")) get_lun( e);
    // par 0 contains now the LUN

    DLong lun;
    e->AssureLongScalarPar( 0, lun);

    bool stdLun = check_lun( e, lun);
    if( stdLun)
      e->Throw( "Unit already open. Unit: "+i2s( lun));
    
    DString host;
    // IDL allows here also arrays of length 1
    e->AssureScalarPar<DStringGDL>( 1, host); 

    DUInt port;
    BaseGDL* p2 = e->GetParDefined( 2);
    if (p2->Type() == STRING) {
      // look up /etc/services
    } else if (p2->Type() == UINT) {
      e->AssureScalarPar<DUIntGDL>( 2, port);
    } else if (p2->Type() == INT) {
      DInt p;
      e->AssureScalarPar<DIntGDL>( 2, p);
      port = p;
    } else if (p2->Type() == LONG) {
      DLong p;
      e->AssureScalarPar<DLongGDL>( 2, p);
      port = p;
    } else if (p2->Type() == ULONG) {
      DULong p;
      e->AssureScalarPar<DULongGDL>( 2, p);
      port = p;
    }

    // endian
    bool swapEndian=false;
    if( e->KeywordSet( "SWAP_ENDIAN"))
      swapEndian = true;
    else if( BigEndian())
      swapEndian = e->KeywordSet( "SWAP_IF_BIG_ENDIAN");
    else
      swapEndian = e->KeywordSet( "SWAP_IF_LITTLE_ENDIAN");

    DDouble c_timeout=0.0;
    e->AssureDoubleScalarKWIfPresent( "CONNECT_TIMEOUT", c_timeout);
    DDouble r_timeout=0.0;
    e->AssureDoubleScalarKWIfPresent( "READ_TIMEOUT",    r_timeout);
    DDouble w_timeout=0.0;
    e->AssureDoubleScalarKWIfPresent( "WRITE_TIMEOUT",   w_timeout);
   
    static int errorIx = e->KeywordIx( "ERROR");
    bool errorKeyword = e->KeywordPresent( errorIx);
    if( errorKeyword) e->AssureGlobalKW( errorIx);

    DLong width = defaultStreamWidth;
    static int widthIx = e->KeywordIx( "WIDTH");
    BaseGDL* widthKeyword = e->GetKW( widthIx);
    if( widthKeyword != NULL)
      {
	e->AssureLongScalarKW( widthIx, width);
      }

    try{
      fileUnits[ lun-1].Socket( host, port, swapEndian,
				c_timeout, r_timeout, c_timeout);
    } 
    catch( GDLException& ex) {
      DString errorMsg = ex.toString()+" Unit: "+i2s( lun)+
	", File: "+fileUnits[ lun-1].Name();
      
      if( !errorKeyword)
	e->Throw( errorMsg);
      
      BaseGDL** err = &e->GetKW( errorIx);
      
      delete (*err); 
//    if( *err != e->Caller()->Object()) delete (*err); 
      
      *err = new DLongGDL( 1);
      return;
    }

    if( errorKeyword)
      {
	BaseGDL** err = &e->GetKW( errorIx);
      
// 	if( *err != e->Caller()->Object()) delete (*err); 
	delete (*err); 
      
	*err = new DLongGDL( 0);
      }
  }

  void close_free_lun( EnvT* e, bool freeLun)
  {
    DLong journalLUN = SysVar::JournalLUN();

    // within GDL, always lun+1 is used
    if( e->KeywordSet("ALL"))
      for( int p=maxUserLun; p<maxLun; ++p)
	{
	  if( (journalLUN-1) != p)
	    {
	      fileUnits[ p].Close();
	      //	      if( freeLun) 
	      fileUnits[ p].Free();
	    }
	}
    
    if( e->KeywordSet("FILE") || e->KeywordSet("ALL"))
      for( int p=0; p<maxUserLun; ++p)
	{
	  fileUnits[ p].Close();
	  // freeing not necessary as get_lun does not use them
	  //if( freeLun) fileUnits[ p].Free();
	}
    
    int nParam=e->NParam();
    for( int p=0; p<nParam; p++)
      {
	DLong lun;
	e->AssureLongScalarPar( p, lun);
	if( lun > maxLun)
	  e->Throw( "File unit is not within allowed range: "+
		    i2s(lun)+".");
	if( lun < 1)
	  e->Throw( "File unit does not allow this operation."
		    " Unit: "+i2s(lun)+".");

	if( lun == journalLUN)
	  e->Throw(  "Reserved file cannot be closed in this manner. Unit: "+
		     i2s( lun));
	
	fileUnits[ lun-1].Close();
	if( freeLun) fileUnits[ lun-1].Free();
      }
  }

  void close_lun( EnvT* e)
  {
    close_free_lun( e, false);
  }
  
  void free_lun( EnvT* e)
  {
    close_free_lun( e, true);
  }

  void writeu( EnvT* e)
  {
    SizeT nParam=e->NParam( 1);

    DLong lun;
    e->AssureLongScalarPar( 0, lun);

    ostream* os;
    bool f77 = false;
    bool swapEndian = false;
    bool compress = false;
    XDR *xdrs = NULL;

    bool stdLun = check_lun( e, lun);
    if( stdLun)
      {
	if( lun == 0)
	  e->Throw( "Cannot write to stdin. Unit: "+i2s( lun));

	os = (lun == -1)? &cout : &cerr;
      }
    else
      {
	os = &fileUnits[ lun-1].OStream();
	f77 = fileUnits[ lun-1].F77();
	swapEndian = fileUnits[ lun-1].SwapEndian();
	compress = fileUnits[ lun-1].Compress();
	xdrs = fileUnits[ lun-1].Xdr();
      }

    if( f77)
      {
	// count record length
	SizeT nBytesAll = 0;
	for( SizeT i=1; i<nParam; i++)
	  {
	    BaseGDL* p = e->GetParDefined( i);
	    nBytesAll += p->NBytes();
	  }
	
	// write record length
	fileUnits[ lun-1].F77Write( nBytesAll);

	// write data
	for( SizeT i=1; i<nParam; i++)
	  {
	    BaseGDL* p = e->GetPar( i); // defined already checked
	    p->Write( *os, swapEndian, compress, xdrs);
	  }

	// write record length
	fileUnits[ lun-1].F77Write( nBytesAll);
      }
    else
      for( SizeT i=1; i<nParam; i++)
	{
	  BaseGDL* p = e->GetParDefined( i);
	  p->Write( *os, swapEndian, compress, xdrs);
	}
  }

  void readu( EnvT* e)
  {
    SizeT nParam=e->NParam( 1);

    DLong lun;
    e->AssureLongScalarPar( 0, lun);

    istream* is;
    igzstream* igzs;
    bool f77 = false;
    bool varlenVMS = false;
    bool swapEndian = false;
    bool compress = false;
    XDR *xdrs = NULL;
    int sockNum = fileUnits[ lun-1].SockNum();

    bool stdLun = check_lun( e, lun);
    if( stdLun)
      {
	if( lun != 0)
	  e->Throw( "Cannot read from stdout and stderr."
		    " Unit: "+i2s( lun));
	is = &cin;
      }
    else if (sockNum != -1)
      {
	// Socket Read
	swapEndian = fileUnits[ lun-1].SwapEndian();

	compress = fileUnits[ lun-1].Compress();

	string *recvBuf = &fileUnits[ lun-1].RecvBuf();

	// Setup recv buffer & string
	const int MAXRECV = 2048*8;
	char buf[MAXRECV+1];

	// Read socket until finished & store in recv string
	while (1) {
	  memset (buf, 0, MAXRECV+1);
	  int status = recv(sockNum, buf, MAXRECV, 0);
	  //	  cout << "Bytes received: " << status << endl;
	  if (status == 0) break;
	  for( SizeT i=0; i<status; i++) 
	    recvBuf->push_back(buf[i]);
	}

	// Get istringstream, write recv string, & assign to istream
	istringstream *iss = &fileUnits[ lun-1].ISocketStream();
	iss->str(*recvBuf);
	is = iss;
      }
    else
      {
	is = &fileUnits[ lun-1].IStream();
	igzs = &fileUnits[ lun-1].IgzStream();
	f77 = fileUnits[ lun-1].F77();
	varlenVMS = fileUnits[ lun-1].VarLenVMS();
	swapEndian = fileUnits[ lun-1].SwapEndian();
	compress = fileUnits[ lun-1].Compress();
	xdrs = fileUnits[ lun-1].Xdr();
      }


    if( f77)
      {
	SizeT recordLength = fileUnits[ lun-1].F77ReadStart();

	SizeT relPos = 0;
	for( SizeT i=1; i<nParam; i++)
	  {
	    BaseGDL* p = e->GetPar( i);
	    if( p == NULL)
	      {
		e->AssureGlobalPar( i);
		p = new DFloatGDL( 0.0);
		e->SetPar( i, p);
	      }

	    SizeT nBytes = p->NBytes();

	    if( (relPos + nBytes) > recordLength)
	      e->Throw( "Attempt to read past end of F77_UNFORMATTED "
			"file record.");

	    p->Read( *is, swapEndian, compress, xdrs);

	    relPos += nBytes;
	  }

	// forward to next record if necessary
	fileUnits[ lun-1].F77ReadEnd();
      }
    else
      for( SizeT i=1; i<nParam; i++)
	{
	  BaseGDL* p = e->GetPar( i);
	  //	  cout << p->Rank() << endl; // JMG
	  if( p == NULL)
	    {
	      e->AssureGlobalPar( i);
	      p = new DFloatGDL( 0.0);
	      e->SetPar( i, p);
	    }

	  if (compress) {
	    p->Read( *igzs, swapEndian, compress, xdrs);
	  } else if (varlenVMS && i == 1) {
	    // Check if VMS variable-length file
	    char hdr[4], tmp;

	    // Read possible record header
	    is->read(hdr, 4);

	    DLong nRec1;
	    memcpy(&nRec1, hdr, 4);

	    // switch endian
	    tmp = hdr[3]; hdr[3] = hdr[0]; hdr[0] = tmp;
	    tmp = hdr[2]; hdr[2] = hdr[1]; hdr[1] = tmp;

	    DLong nRec2;
	    memcpy(&nRec2, hdr, 4);
	    SizeT nBytes = p->NBytes();

	    // In variable length VMS files, each record is prefixed 
	    // with a count byte that contains the number of bytes 
	    // in the record.  This step checks whether the length
	    // of the possible header record actually corresponds
	    // to the total length of the desired fields in the
	    // call to READU.

	    // if not VMS v.l.f then backup 4 bytes and tag files
	    // as not variable-length
	    if (nRec1 != nBytes && nRec2 != nBytes) { 	     
	      is->seekg(-4, ios::cur);
	      fileUnits[ lun-1].PutVarLenVMS( false);
	    }
	    p->Read( *is, swapEndian, compress, xdrs);
	  }
	  else p->Read( *is, swapEndian, compress, xdrs);

	  // Socket Read
	  if (sockNum != -1) {
	    int pos = is->tellg();
	    string *recvBuf = &fileUnits[ lun-1].RecvBuf();
	    //	    cout << "pos: " << pos << endl;
	    recvBuf->erase(0, pos);
	  }
	}
  }

  void on_error( EnvT* e)
  {
    e->OnError();
  }

  void catch_pro( EnvT* e)
  {
    e->Catch();
  }

  void strput( EnvT* e)
  {
    SizeT nParam = e->NParam( 2);
    
    DStringGDL* dest = dynamic_cast<DStringGDL*>( e->GetParGlobal( 0));
    if( dest == NULL)
      e->Throw( "String expression required in this context: "+
		e->GetParString(0));
    
    DString source;
    e->AssureStringScalarPar( 1, source);
    
    DLong pos = 0;
    if (nParam == 3)
      {
	e->AssureLongScalarPar( 2, pos);
	if (pos < 0) pos = 0;
      }

    SizeT nEl = dest->N_Elements();
    for( SizeT i=0; i<nEl; ++i)
	StrPut((*dest)[ i], source, pos);
  }

  void retall( EnvT* e)
  {
    e->Interpreter()->RetAll();
  }

  void stop( EnvT* e)
  {
    if( e->NParam() > 0) print( e);
    debugMode = DEBUG_STOP;
  }

  void defsysv( EnvT* e)
  {
    SizeT nParam = e->NParam( 1);

    DString sysVarNameFull;
    e->AssureStringScalarPar( 0, sysVarNameFull);
    
    static int existIx = e->KeywordIx( "EXIST");
    if( e->KeywordPresent( existIx))
      {
	if( sysVarNameFull.length() < 2 || sysVarNameFull[0] != '!')
	  {
	    e->SetKW( existIx, new DLongGDL( 0));
	  }
	
	DVar* sysVar = FindInVarList( sysVarList,
				      StrUpCase( sysVarNameFull.substr(1)));
	if( sysVar == NULL)
	  e->SetKW( existIx, new DLongGDL( 0));
	else
	  e->SetKW( existIx, new DLongGDL( 1));
	return;
      }
    else if( nParam < 2)
      e->Throw( "Incorrect number of arguments.");
    
    // here: nParam >= 2
    DLong rdOnly = 0;
    if( nParam >= 3)
      e->AssureLongScalarPar( 2, rdOnly);

    if( sysVarNameFull.length() < 2 || sysVarNameFull[0] != '!')
      e->Throw( "Illegal system variable name: "+sysVarNameFull+".");
    
    // strip "!", uppercase
    DString sysVarName = StrUpCase( sysVarNameFull.substr(1)); 
    
    DVar* sysVar = FindInVarList( sysVarList, sysVarName);

    // check if the variable is defined
    BaseGDL* p1 = e->GetParDefined( 1);

    if( sysVar == NULL)
      {
	// define new
	DVar *newSysVar = new DVar( sysVarName, p1->Dup());
	sysVarList.push_back( newSysVar);

	// rdOnly is only set at the first definition
	if( rdOnly != 0)
	  sysVarRdOnlyList.push_back( newSysVar);
	return;
      }

    // re-set
    // make sure type and size are kept
    BaseGDL* oldVar = sysVar->Data();
    BaseGDL* newVar = p1;
    if( oldVar->Type()       != newVar->Type() ||
	oldVar->N_Elements() != newVar->N_Elements())
      e->Throw( "Conflicting definition for "+sysVarNameFull+".");

    // if struct -> assure equal descriptors
    DStructGDL *oldStruct =  dynamic_cast<DStructGDL*>( oldVar);
    if( oldStruct != NULL)
      {
	// types are same -> static cast
	DStructGDL *newStruct =  static_cast<DStructGDL*>( newVar);

	// note that IDL handles different structs more relaxed
	// ie. just the structure pattern is compared.
	if( *oldStruct->Desc() != *newStruct->Desc())
	  e->Throw( "Conflicting definition for "+sysVarNameFull+".");
      }
	
    DVar* sysVarRdOnly = FindInVarList( sysVarRdOnlyList, sysVarName);
    if( sysVarRdOnly != NULL)
      {
	// rdOnly set and is already rdOnly: do nothing
	if( rdOnly != 0) return; 

	// else complain
	e->Throw( "Attempt to write to a readonly variable: "+
		  sysVarNameFull+".");
      }
    else
      {
	// not read only
	delete oldVar;
	sysVar->Data() = newVar->Dup();

	// only on first definition
	//	if( rdOnly != 0)
	//	  sysVarRdOnlyList.push_back( sysVar);
      }
  }

  // note: this implemetation does not honor all keywords
  void message( EnvT* e)
  {
    SizeT nParam = e->NParam();

    if( nParam == 0) return;

    static int continueIx = e->KeywordIx( "CONTINUE");
    static int infoIx = e->KeywordIx( "INFORMATIONAL");
    static int ioerrorIx = e->KeywordIx( "IOERROR");
    static int nonameIx = e->KeywordIx( "NONAME");
    static int noprefixIx = e->KeywordIx( "NOPREFIX");
    static int noprintIx = e->KeywordIx( "NOPRINT");
    static int resetIx = e->KeywordIx( "RESET");

    bool continueKW = e->KeywordSet( continueIx);
    bool info = e->KeywordSet( infoIx);
    bool ioerror = e->KeywordSet( ioerrorIx);
    bool noname = e->KeywordSet( nonameIx);
    bool noprefix = e->KeywordSet( noprefixIx);
    bool noprint = e->KeywordSet( noprintIx);
    bool reset = e->KeywordSet( resetIx);

    DString msg;
    e->AssureScalarPar<DStringGDL>( 0, msg);

    if( !noname)
      msg = e->Caller()->GetProName() + ": " + msg;

    if( !info)
      {
	DStructGDL* errorState = SysVar::Error_State();
	static unsigned msgTag = errorState->Desc()->TagIndex( "MSG");
	(*static_cast<DStringGDL*>( errorState->GetTag( msgTag)))[0] = msg;
	
	SysVar::SetErr_String( msg);
      }
	
    if( noprint)
      msg = "";
    
    if( !continueKW && !info)
      throw GDLException( msg, !noprefix);
    
    if( !noprint && !noprefix)
      msg = SysVar::MsgPrefix() + msg;

    if( !info || (SysVar::Quiet() == 0))
      cout << msg << endl;
  }

  void byteorder( EnvT* e)
  {
    SizeT nParam = e->NParam( 1);

    //    static int sswapIx = e->KeywordIx( "SSWAP");
    static int lswapIx = e->KeywordIx( "LSWAP");
    static int l64swapIx = e->KeywordIx( "L64SWAP");
    static int ifBigIx = e->KeywordIx( "SWAP_IF_BIG_ENDIAN");
    static int ifLittleIx = e->KeywordIx( "SWAP_IF_LITTLE_ENDIAN");
    static int ntohlIx = e->KeywordIx( "NTOHL");
    static int ntohsIx = e->KeywordIx( "NTOHS");
    static int htonlIx = e->KeywordIx( "HTONL");
    static int htonsIx = e->KeywordIx( "HTONS");
    static int ftoxdrIx = e->KeywordIx( "FTOXDR");
    static int dtoxdrIx = e->KeywordIx( "DTOXDR");
    static int xdrtofIx = e->KeywordIx( "XDRTOF");
    static int xdrtodIx = e->KeywordIx( "XDRTOD");

    bool lswap = e->KeywordSet( lswapIx);
    bool l64swap = e->KeywordSet( l64swapIx);
    bool ifBig = e->KeywordSet( ifBigIx);
    bool ifLittle = e->KeywordSet( ifLittleIx);

    // to-from network conversion (big-endian)
    bool ntohl = e->KeywordSet( ntohlIx);
    bool ntohs = e->KeywordSet( ntohsIx);
    bool htonl = e->KeywordSet( htonlIx);
    bool htons = e->KeywordSet( htonsIx);

    // XDR to-from conversion
    bool ftoxdr = e->KeywordSet( ftoxdrIx);
    bool dtoxdr = e->KeywordSet( dtoxdrIx);
    bool xdrtof = e->KeywordSet( xdrtofIx);
    bool xdrtod = e->KeywordSet( xdrtodIx);

    if( ifBig && !BigEndian()) return;
    if( ifLittle && BigEndian()) return;

    if( BigEndian() && (ntohl || ntohs || htonl || htons)) return;

    for( DLong p=nParam-1; p>=0; --p)
      {
	BaseGDL* par = e->GetParDefined( p);
	if( !e->GlobalPar( p))
	  e->Throw( "Expression must be named variable in this context: "+
		    e->GetParString(p));		    

	SizeT nBytes = par->NBytes();
	char* addr = static_cast<char*>(par->DataAddr());
	
	SizeT swapSz = 2; 
	if( l64swap || dtoxdr || xdrtod)
	  swapSz = 8;
	else if( lswap || ntohl || htonl || ftoxdr || xdrtof)
	  swapSz = 4;

	if( nBytes % swapSz != 0)
	  e->Throw( "Operand's size must be a multiple of swap "
		    "datum size: " + e->GetParString(p));		    
	    
	SizeT nSwap = nBytes / swapSz;

	for( SizeT i=0; i<nSwap; ++i)
	  {
	    for( SizeT s=0; s < (swapSz/2); ++s)
	      {
		char tmp = *(addr+i*swapSz+s);
		*(addr+i*swapSz+s) = *(addr+i*swapSz+swapSz-1-s);
		*(addr+i*swapSz+swapSz-1-s) = tmp;
	      }
	  }
      }
  }

  void setenv_pro( EnvT* e)
  {
    SizeT nParam = e->NParam();

    DStringGDL* name = e->GetParAs<DStringGDL>(0);
    SizeT nEnv = name->N_Elements();

    for(SizeT i=0; i < nEnv; ++i){
      DString strEnv = (*name)[i];
      long len = strEnv.length();
      long pos = strEnv.find_first_of("=", 0); 
      if( pos == string::npos) continue;   
      DString strArg = strEnv.substr(pos+1, len - pos - 1);
      strEnv = strEnv.substr(0, pos);
      // putenv() is POSIX unlike setenv()
      #if defined(__hpux__)
      int ret = putenv((strEnv+"="+strArg).c_str());
      #else
      int ret = setenv(strEnv.c_str(), strArg.c_str(), 1);
      #endif
    }
  } 

  void struct_assign_pro( EnvT* e)
  {
    SizeT nParam=e->NParam( 2);
 
    DStructGDL* source = e->GetParAs<DStructGDL>(0);
    DStructGDL* dest   = e->GetParAs<DStructGDL>(1);
  
    static int nozeroIx = e->KeywordIx("NOZERO");
    bool nozero = e->KeywordSet( nozeroIx); 

    static int verboseIx = e->KeywordIx("VERBOSE");
    bool verbose = e->KeywordSet( verboseIx);

    string sourceName = (*source).Desc()->Name();

    SizeT nTags = 0;

    // array of struct
    SizeT nElements = source->N_Elements();
    SizeT nDestElements = dest->N_Elements();
    if( nElements > nDestElements)
      nElements = nDestElements;

    // zero out the destination
    if( !nozero)
       (*dest).Clear();

    nTags = (*source).Desc()->NTags();

    // copy the stuff
    for(int t=0; t < nTags; ++t)
      {    
	string sourceTagName = (*source).Desc()->TagName(t);
	int ix = (*dest).Desc()->TagIndex( sourceTagName );
	if( ix >= 0)
	  {
	    SizeT nTagElements = source->GetTag( t)->N_Elements();
	    SizeT nTagDestElements = dest->GetTag( ix)->N_Elements();

	    if( verbose) 
	      {
		if( nTagElements > nTagDestElements)
		  Warning( "STRUCT_ASSIGN: " + sourceName + 
			   " tag " + sourceTagName + 
			   " is longer than destination. "
			   "The end will be clipped.");
		else if( nTagElements < nTagDestElements)
		  Warning( "STRUCT_ASSIGN: " + sourceName + 
			   " tag " + sourceTagName + 
			   " is shorter than destination. "
			   "The end will be zero filled.");
	      }

	    if( nTagElements > nTagDestElements)
		nTagElements = nTagDestElements;

	    for( SizeT a=0; a< nElements; ++a)
	      dest->GetTag( ix, a)->Assign( source->GetTag( t, a), nTagElements);
	  }
	else 
	  if(verbose)
	    Warning( "STRUCT_ASSIGN: Destination lacks " + sourceName +
		     " tag " + sourceTagName + ". Not copied.");
      }
  }

  void spawn_pro( EnvT* e)
  {
    SizeT nParam = e->NParam();

    static int countIx = e->KeywordIx( "COUNT");
    bool countKeyword = e->KeywordPresent( countIx);
    if( countKeyword) e->AssureGlobalKW( countIx);

    static int pidIx = e->KeywordIx( "PID");
    bool pidKeyword = e->KeywordPresent( pidIx);
    if( pidKeyword) e->AssureGlobalKW( pidIx);
    
    static int exit_statusIx = e->KeywordIx( "EXIT_STATUS");
    bool exit_statusKeyword = e->KeywordPresent( exit_statusIx);
    if( exit_statusKeyword) e->AssureGlobalKW( exit_statusIx);

    static int shIx = e->KeywordIx( "SH");
    bool shKeyword = e->KeywordSet( shIx);
    
    static int noshellIx = e->KeywordIx( "NOSHELL");
    bool noshellKeyword = e->KeywordSet( noshellIx);
    
    string shellCmd;
    if( shKeyword) 
      shellCmd = "/bin/sh"; // must be there if POSIX
    else
      {
	shellCmd= getenv("SHELL");
	if(shellCmd == "")
	  e->Throw( "Error managing child process. "
		    "Environment variable SHELL not set.");
      }

    if( nParam == 0)
      { 
	system( shellCmd.c_str());
	if( countKeyword)
	  e->SetKW( countIx, new DLongGDL( 0));
	return;
      }

    DStringGDL* command = e->GetParAs<DStringGDL>( 0);
    DString cmd = (*command)[0];

    const int bufSize = 1024;
    char buf[ bufSize];

    if( nParam > 1) e->AssureGlobalPar( 1);
    if( nParam > 2) e->AssureGlobalPar( 2);

    int coutP[2];
    if( nParam > 1 && pipe(coutP)) return;	

    int cerrP[2];
    if( nParam > 2 && pipe(cerrP)) return;	
	
    pid_t pid = fork(); // *** fork
    if( pid == -1) // error in fork
      {
	close( coutP[0]); close( coutP[1]);
	if( nParam > 2) { close( cerrP[0]); close( cerrP[1]);}
	return;
      }

    if( pid == 0) // we are child
      {
	if( nParam > 1) dup2(coutP[1], 1); // cout
	if( nParam > 2) dup2(cerrP[1], 2); // cerr

	if( nParam > 1) { close( coutP[0]); close( coutP[1]);}
	if( nParam > 2) { close( cerrP[0]); close( cerrP[1]);}

	if( noshellKeyword)
	  {
	    SizeT nArg = command->N_Elements();
	    char** argv = new char*[ nArg+1];
	    argv[ nArg] = NULL;
	    for( SizeT i=0; i<nArg; ++i)
	      argv[i] = const_cast<char*>((*command)[i].c_str());
		
	    execvp( cmd.c_str(), argv);

	    delete[] argv; // only executes if exec fails
	  }
	else
	  execl( shellCmd.c_str(), shellCmd.c_str(), "-c", 
		 cmd.c_str(), (char *) NULL);

	Warning( "SPAWN: Error managing child process.");
	_exit(1); // error in exec
      }
    else // we are parent
      {
	if( pidKeyword)
	  e->SetKW( pidIx, new DLongGDL( pid));

	if( nParam > 1) close( coutP[1]);
	if( nParam > 2) close( cerrP[1]);

	FILE *coutF, *cerrF;
	if( nParam > 1) 
	  {
	    coutF = fdopen( coutP[0], "r");
	    if( coutF == NULL) close( coutP[0]);
	  }
	if( nParam > 2) 
	  {
	    cerrF = fdopen( cerrP[0], "r");
	    if( cerrF == NULL) close( cerrP[0]);
	  }

	vector<DString> outStr;
	vector<DString> errStr;
	    
	// read cout
	if( nParam > 1 && coutF != NULL)
	  {
	    while( fgets(buf, bufSize, coutF) != NULL)
	      {
		SizeT len = strlen( buf);
		if( len != 0 && buf[ len-1] == '\n') 
		  buf[ len-1] = 0;
		outStr.push_back( DString( buf));
	      }
	    fclose( coutF);
	  }

	// read cerr
	if( nParam > 2 && cerrF != NULL) 
	  {
	    while( fgets(buf, bufSize, cerrF) != NULL)
	      {
		SizeT len = strlen( buf);
		if( len != 0 && buf[ len-1] == '\n') 
		  buf[ len-1] = 0;
		errStr.push_back( DString( buf));
	      }
	    fclose( cerrF);
	  }

	// wait until child terminates
	int status;
	pid_t wpid  = wait( &status);
	
	if( exit_statusKeyword)
	  e->SetKW( exit_statusIx, new DLongGDL( status));
	    
	SizeT nLines = 0;
	if( nParam > 1)
	  {
	    DStringGDL* result;
	    nLines = outStr.size();
	    if( nLines == 0)
	      result = new DStringGDL("");
	    else 
	      {
		result = new DStringGDL( dimension( nLines), 
					 BaseGDL::NOZERO);
		for( SizeT l=0; l<nLines; ++l)
		  (*result)[ l] = outStr[ l];
	      }
	    e->SetPar( 1, result);
	  }

	if( countKeyword)
	  e->SetKW( countIx, new DLongGDL( nLines));
	    
	if( nParam > 2)
	  {
	    DStringGDL* errResult;
	    SizeT nErrLines = errStr.size();
	    if( nErrLines == 0)
	      errResult = new DStringGDL("");
	    else 
	      {
		errResult = new DStringGDL( dimension( nErrLines), 
					    BaseGDL::NOZERO);
		for( SizeT l=0; l<nErrLines; ++l)
		  (*errResult)[ l] = errStr[ l];
	      }
	    e->SetPar( 2, errResult);
	  }

	return;
      }
  }

  void replicate_inplace_pro( EnvT* e)
  {
    SizeT nParam = e->NParam( 2);

    if( nParam % 2)
      e->Throw( "Incorrect number of arguments.");

    BaseGDL* p0 = e->GetParDefined( 0);
    if( !e->GlobalPar( 0))
      e->Throw( "Expression must be named variable in this context: "+
		e->GetParString(0));
    
    BaseGDL* p1 = e->GetParDefined( 1);
    if( !p1->Scalar())
      e->Throw( "Expression must be a scalar in this context: "+
		e->GetParString(1));
    p1 = p1->Convert2( p0->Type(), BaseGDL::COPY);
    
    if (nParam == 2)
      {
        p0->AssignAt( p1);
	return;
      } 
    else 
      {
 	BaseGDL* p2 = e->GetNumericParDefined( 2);
	if( !p2->StrictScalar())
	  e->Throw( "Expression must be a scalar in this context: "+
		    e->GetParString(2));
	
	SizeT d1;
	int ret = p2->Scalar2index( d1);
	if( d1 < 1 || d1 > p0->Rank())
	  e->Throw( "D1 (3rd) argument is out of range: "+
		    e->GetParString(2));
	
// 	BaseGDL* p3 = e->GetNumericParDefined( 3);
	DLongGDL* p3 = e->GetParAs< DLongGDL>( 3);
	if( p3->N_Elements() != p0->Rank())
	  e->Throw( "Loc1 (4th) argument must have the same number of "
		    "elements as the dimensions of the X (1st) argument: "+
		    e->GetParString(3));

	SizeT d2 = 0;
	BaseGDL* p4;
	BaseGDL* p5;
	if (nParam > 4) 
	  {
	    p4 = e->GetNumericParDefined( 4);
	    if( !p4->StrictScalar())
	      e->Throw( "Expression must be a scalar in this context: "+
		        e->GetParString(4));
	    ret = p4->Scalar2index( d2);
	    if( d2 < 1 || d2 > p0->Rank())
	      e->Throw( "D5 (5th) argument is out of range: "+
		        e->GetParString(4));

	    p5 = e->GetNumericParDefined( 5);
	  }

	ArrayIndexVectorT* ixList = new ArrayIndexVectorT();
	auto_ptr< ArrayIndexVectorT> ixList_guard( ixList);
// 	BaseGDL* loc1 = p3->Dup();
// 	loc1->SetDim (dimension( loc1->N_Elements()));
	ixList->reserve( p3->N_Elements());
	for (size_t i=0; i<p3->N_Elements(); i++)
	  if( (i+1) == d1)
	    ixList->push_back( new ArrayIndexAll());
	  else if( (i+1) == d2)
	    ixList->push_back( new CArrayIndexIndexed( p5, true));
	  else
	    ixList->push_back( new CArrayIndexScalar( (*p3)[ i]));//p3->NewIx(i)));
	ArrayIndexListT* ixL = MakeArrayIndex( ixList);
	auto_ptr< ArrayIndexListT> ixL_guard( ixL);
	ixL->AssignAt( p0, p1);	
	return;
      }
  }
  
} // namespace
