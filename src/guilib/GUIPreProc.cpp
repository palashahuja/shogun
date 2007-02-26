/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2007 Soeren Sonnenburg
 * Written (W) 1999-2007 Gunnar Raetsch
 * Copyright (C) 1999-2007 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "lib/config.h"

#ifndef HAVE_SWIG
#include "guilib/GUIPreProc.h"
#include "gui/GUI.h"
#include "preproc/LogPlusOne.h"
#include "preproc/NormOne.h"
#include "preproc/PruneVarSubMean.h"
#include "preproc/PCACut.h"
#include "preproc/SortWord.h"
#include "preproc/SortWordString.h"
#include "preproc/SortUlongString.h"
#include "features/RealFileFeatures.h"
#include "features/TOPFeatures.h"
#include "features/FKFeatures.h"
#include "features/CharFeatures.h"
#include "features/StringFeatures.h"
#include "features/ByteFeatures.h"
#include "features/ShortFeatures.h"
#include "features/RealFeatures.h"
#include "features/SparseFeatures.h"
#include "features/CombinedFeatures.h"
#include "features/Features.h"
#include "lib/io.h"
#include "lib/config.h"

#include <string.h>
#include <stdio.h>

CGUIPreProc::CGUIPreProc(CGUI * gui_)
  : CSGObject(), gui(gui_)
{
	preprocs=new CList<CPreProc*>(true);
	attached_preprocs_lists=new CList<CList<CPreProc*>*>(true);
}

CGUIPreProc::~CGUIPreProc()
{
	delete preprocs;
	delete attached_preprocs_lists;
}


bool CGUIPreProc::add_preproc(CHAR* param)
{
	CPreProc* preproc=NULL;

	param=io.skip_spaces(param);
#ifdef HAVE_LAPACK
	if (strncmp(param,"PCACUT",6)==0)
	{
		INT do_whitening=0; 
		double thresh=1e-6 ;
		sscanf(param+6, "%i %le", &do_whitening, &thresh) ;
		SG_INFO( "PCACUT parameters: do_whitening=%i thresh=%e", do_whitening, thresh) ;
		preproc=new CPCACut(do_whitening, thresh);
	}
	else 
#endif
	if (strncmp(param,"NORMONE",7)==0)
	{
		preproc=new CNormOne();
	}
	else if (strncmp(param,"LOGPLUSONE",10)==0)
	{
		preproc=new CLogPlusOne();
	}
	else if (strncmp(param,"SORTWORDSTRING",14)==0)
	{
		preproc=new CSortWordString();
	}
	else if (strncmp(param,"SORTULONGSTRING",15)==0)
	{
		preproc=new CSortUlongString();
	}
	else if (strncmp(param,"SORTWORD",8)==0)
	{
		preproc=new CSortWord();
	}
	else if (strncmp(param,"PRUNEVARSUBMEAN",15)==0)
	{
		INT divide_by_std=0; 
		sscanf(param+15, "%i", &divide_by_std);

		if (divide_by_std)
			SG_INFO( "normalizing VARIANCE\n");
		else
			SG_WARNING( "NOT normalizing VARIANCE\n");

		preproc=new CPruneVarSubMean(divide_by_std==1);
	}
	else 
	{
		io.not_implemented();
		return false;
	}

	preprocs->get_last_element();
	return preprocs->append_element(preproc);
}

bool CGUIPreProc::clean_preproc(CHAR* param)
{
	delete preprocs;
	preprocs=new CList<CPreProc*>(true);
	return (preprocs!=NULL);
}

bool CGUIPreProc::del_preproc(CHAR* param)
{
	SG_INFO( "deleting preproc %i/(%i)\n", preprocs->get_num_elements()-1, preprocs->get_num_elements());


	CPreProc* p=preprocs->delete_element();
	if (p)
		delete p;
	return (p!=NULL);
}

bool CGUIPreProc::load(CHAR* param)
{
	bool result=false;

	param=io.skip_spaces(param);

	CPreProc* preproc=NULL;

	FILE* file=fopen(param, "r");
	CHAR id[5]="UDEF";

	if (file)
	{
		ASSERT(fread(id, sizeof(char), 4, file)==4);
	
#ifdef HAVE_LAPACK
		if (strncmp(id, "PCAC", 4)==0)
		{
			preproc=new CPCACut();
		}
		else 
#endif
		if (strncmp(id, "NRM1", 4)==0)
		{
			preproc=new CNormOne();
		}
		else if (strncmp(id, "PVSM", 4)==0)
		{
			preproc=new CPruneVarSubMean();
		}
		else
			SG_ERROR( "unrecognized file\n");

		if (preproc && preproc->load_init_data(file))
		{
			printf("file successfully read\n");
			result=true;
		}

		fclose(file);
	}
	else
		SG_ERROR( "opening file %s failed\n", param);

	if (result)
	{
		preprocs->get_last_element();
		result=preprocs->append_element(preproc);
	}

	return result;
}

bool CGUIPreProc::save(CHAR* param)
{
	CHAR fname[1024];
	INT num=preprocs->get_num_elements()-1;
	bool result=false; param=io.skip_spaces(param);
	sscanf(param, "%s %i", fname, &num);
	CPreProc* preproc= preprocs->get_last_element();

	if (num>=0 && (num < preprocs->get_num_elements()) && preproc)
	{
		FILE* file=fopen(fname, "w");
	
		fwrite(preproc->get_id(), sizeof(char), 4, file);
		if ((!file) ||	(!preproc->save_init_data(file)))
			printf("writing to file %s failed!\n", param);
		else
		{
			printf("successfully written preproc init data into \"%s\" !\n", param);
			result=true;
		}

		if (file)
			fclose(file);
	}
	else
		SG_ERROR( "create preproc first\n");

	return result;
}

bool CGUIPreProc::attach_preproc(CHAR* param)
{
	bool result=false;
	param=io.skip_spaces(param);
	CHAR target[1024]="";
	INT force=0;

	if ((sscanf(param, "%s %d", target, &force))>=1)
	{
		if ( strcmp(target, "TRAIN")==0 || strcmp(target, "TEST")==0 )
		{
			if (strcmp(target,"TRAIN")==0)
			{
				CFeatures* f = gui->guifeatures.get_train_features();
				if (f->get_feature_class()==C_COMBINED)
					f=((CCombinedFeatures*)f)->get_last_feature_obj();

				preprocess_features(f, NULL, force==1);
				gui->guifeatures.invalidate_train();
				result=true;
			}
			else if (strcmp(target,"TEST")==0)
			{
				CFeatures* f_test = gui->guifeatures.get_test_features();
				CFeatures* f_train  = gui->guifeatures.get_train_features();

				if (f_train->get_feature_class() == f_test->get_feature_class())
				{
					if (f_train->get_feature_class() == C_COMBINED)
					{
						if (((CCombinedFeatures*) f_train)->check_feature_obj_compatibility((CCombinedFeatures*) f_test) )
						{
							//preprocess the last test feature obj
							CFeatures* te_feat = ((CCombinedFeatures*) f_test)->get_first_feature_obj();
							CFeatures* tr_feat = ((CCombinedFeatures*) f_train)->get_first_feature_obj();

							INT num_combined= ((CCombinedFeatures*) f_test)->get_num_feature_obj();
							ASSERT(((CCombinedFeatures*) f_train)->get_num_feature_obj() == num_combined);

							if (!(num_combined && tr_feat && te_feat))
								SG_ERROR( "one of the combined features has no sub-features ?!\n");

							SG_INFO( "BEGIN PREPROCESSING COMBINED FEATURES (%d sub-featureobjects)\n", num_combined);
							
							int n=0;
							while (n<num_combined && tr_feat && te_feat)
							{
								// and preprocess using that one 
								SG_INFO( "TRAIN ");
								tr_feat->list_feature_obj();
								SG_INFO( "TEST ");
								te_feat->list_feature_obj();
								preprocess_features(tr_feat, te_feat, force);

								tr_feat = ((CCombinedFeatures*) f_train)->get_next_feature_obj();
								te_feat = ((CCombinedFeatures*) f_test)->get_next_feature_obj();
								n++;
							}
							ASSERT(n==num_combined);
							result=true;
							SG_INFO( "END PREPROCESSING COMBINED FEATURES\n");
						}
						else
							SG_ERROR( "combined features not compatible\n");
					}
					else
					{
						preprocess_features(f_train, f_test, force==1);
						gui->guifeatures.invalidate_test();
						result=true;
					}
				}
				else
					SG_ERROR( "features not compatible\n");
			}
			else
				SG_ERROR( "see help for parameters\n");
		}
		else
			SG_ERROR( "features not correctly assigned!\n");
	}
	else
		SG_ERROR( "see help for parameters\n");

	/// when successful add preprocs to attached_preprocs list (for removal later)
	/// and clean the current preproc list
	if (result)
	{
		attached_preprocs_lists->get_last_element();
		attached_preprocs_lists->append_element(preprocs);
		preprocs=new CList<CPreProc*>(true);
	}

	return result;
}

bool CGUIPreProc::preprocess_features(CFeatures* trainfeat, CFeatures* testfeat, bool force)
{
	if (trainfeat)
	{
		if (testfeat)
		{
			// if we don't have a preproc for trainfeatures we 
			// don't need a preproc for test features
			SG_DEBUG( "%d preprocessors attached to train features %d to test features\n", trainfeat->get_num_preproc(), testfeat->get_num_preproc());

			if (trainfeat->get_num_preproc() < testfeat->get_num_preproc())
			{
				SG_ERROR( "more preprocessors attached to test features than to train features\n");
				return false;
			}

			if (trainfeat->get_num_preproc() && (trainfeat->get_num_preproc() > testfeat->get_num_preproc()))
			{
				for (INT i=0; i<trainfeat->get_num_preproc();  i++)
				{
					CPreProc* preproc = trainfeat->get_preproc(i);
					preproc->init(trainfeat);
					testfeat->add_preproc(trainfeat->get_preproc(i));
				}

				preproc_all_features(testfeat, force);
			}
		}
		else
		{
			CPreProc* preproc = preprocs->get_first_element();

			if (preproc)
			{
				preproc->init(trainfeat);
				trainfeat->add_preproc(preproc);

				preproc_all_features(trainfeat, force);
			}

			while ( (preproc = preprocs->get_next_element()) !=NULL )
			{
				preproc->init(trainfeat);
				trainfeat->add_preproc(preproc);

				preproc_all_features(trainfeat, force);
			}
		}

		return true;
	}
	else
		SG_ERROR( "no features for preprocessing available!\n");

	return false;
}

bool CGUIPreProc::preproc_all_features(CFeatures* f, bool force)
{
	switch (f->get_feature_class())
	{
		case C_SIMPLE:
			switch (f->get_feature_type())
			{
				case F_DREAL:
					return ((CRealFeatures*) f)->apply_preproc(force);
				case F_SHORT:
					return ((CShortFeatures*) f)->apply_preproc(force);
				case F_WORD:
					return ((CShortFeatures*) f)->apply_preproc(force);
				case F_CHAR:
					return ((CCharFeatures*) f)->apply_preproc(force);
				case F_BYTE:
					return ((CByteFeatures*) f)->apply_preproc(force);
				default:
					io.not_implemented();
			}
			break;
		case C_STRING:
			switch (f->get_feature_type())
			{
				case F_WORD:
					return ((CStringFeatures<WORD>*) f)->apply_preproc(force);
				case F_ULONG:
					return ((CStringFeatures<ULONG>*) f)->apply_preproc(force);
				default:
					io.not_implemented();
			}
			break;
		case C_SPARSE:
			switch (f->get_feature_type())
			{
				case F_DREAL:
					return ((CSparseFeatures<DREAL>*) f)->apply_preproc(force);
				default:
					io.not_implemented();
			};
			break;
		case C_COMBINED:
			SG_ERROR( "Combined feature objects cannot be preprocessed. Only its sub-feature objects!\n");
			break;
		default:
			io.not_implemented();
	}

	return false;
}
#endif
