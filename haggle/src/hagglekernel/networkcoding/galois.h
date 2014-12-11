/*
* Copyright (c) 2006-2013,  University of California, Los Angeles
* Coded by Joe Yeh/Uichin Lee
* Modified and extended by Seunghoon Lee/Sung Chul Choi
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
* in the documentation and/or other materials provided with the distribution.
* Neither the name of the University of California, Los Angeles nor the names of its contributors 
* may be used to endorse or promote products derived from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __GALOIS_H__
#define __GALOIS_H__

#include <stdio.h>

#define GF8  256
#define PP8 0435 // n.b. octal value.
// Galois module
class Galois {

public:
	////////////////////////////
	// constructor
	Galois() :
			GF(GF8), PP(PP8) {

		Init();
	}

	// destructor
	~Galois() {

	}

	// operations
	inline unsigned char Add(unsigned char a, unsigned char b, int ff) {
		//printf("add a=|%u| b=|%u| ff=|%d|\n",a,b,ff);
		return a ^ b;
	}
	; // a+b
	inline unsigned char Sub(unsigned char a, unsigned char b, int ff) {
		//printf("sub a=|%u| b=|%u| ff=|%d|\n",a,b,ff);
		return a ^ b;
	}
	; // a-b
//	unsigned char Mul(unsigned char a, unsigned char b, int ff);		// a*b
//	unsigned char Div(unsigned char a, unsigned char b, int ff);		// a/b
	inline unsigned char Mul(unsigned char a, unsigned char b, int ff) {
		//printf("mul a=|%u| b=|%u| ff=|%d|\n",a,b,ff);
		if (a == 0 || b == 0)
			return 0;
		else
			//return ALog[(Log[a]+Log[b])%(GF-1)]; w/o optimization
			return ALog[Log[a] + Log[b]];
	}

	inline unsigned char Div(unsigned char a, unsigned char b, int ff) {
		//printf("div a=|%u| b=|%u| ff=|%d|\n",a,b,ff);
		if (a == 0 || b == 0)
			return 0;
		else
			//return ALog[(Log[a]-Log[b]+GF-1)%(GF-1)]; w/o optimization
			return ALog[Log[a] - Log[b] + GF - 1];
	}
private:

	////////////////////////////
	// member variables

	// log tables
	unsigned char Log[GF8];
	unsigned char ALog[GF8 * 2];

	const int GF;
	const int PP;

	////////////////////////////
	// private functions

	// Initializes log tables
	//void Init();
	void Init() {
		int b, j;

		for (j = 0; j < GF; j++) {
			Log[j] = GF - 1;
			ALog[j] = 0;
		}

		b = 1;
		for (j = 0; j < GF - 1; j++) {
			Log[b] = j;
			ALog[j] = b;
			b = b * 2;
			if (b & GF)
				b = (b ^ PP) & (GF - 1);
		}

		for (; j <= (GF - 1) * 2 - 2; j++) {
			ALog[j] = ALog[j % (GF - 1)];
		}

		ALog[(GF - 1) * 2 - 1] = ALog[254];

	}

};

#endif
