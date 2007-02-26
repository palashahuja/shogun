/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2007 Soeren Sonnenburg
 * Copyright (C) 1999-2007 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#ifndef _GPBTSVM_H___
#define _GPBTSVM_H___
#include "lib/common.h"
#include "classifier/svm/SVM.h"
#include "classifier/svm/SVM_libsvm.h"

#include <stdio.h>

class CGPBTSVM : public CSVM
{
	public:
		CGPBTSVM();
		CGPBTSVM(DREAL C, CKernel* k, CLabels* lab);
		virtual ~CGPBTSVM();
		virtual bool train();
		inline EClassifierType get_classifier_type() { return CT_GPBT; }

	protected:
		struct svm_model* model;
};
#endif
