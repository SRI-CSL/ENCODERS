/*
* Copyright (c) 2006-2013,  University of California, Los Angeles
* Coded by Joe Yeh/Uichin Lee
* Modified and extended by Seunghoon Lee/Sung Chul Choi/Yu-Ting Yu
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
#include <errno.h>
#include "codetorrent.h"
//#include "main.cpp"
//for time-stamp
#include <sys/timeb.h>
#include <sys/time.h>
//#include <time.h>

#include <Trace.h>

struct timeval ct_tv_beg, ct_tv_end;
struct timeval ct_encode_beg, ct_encode_end;
struct timeval ct_vector_beg, ct_vector_end;

//void* tz;
struct timezone ct_tz;

// Server (Data source)
CodeTorrent::CodeTorrent(int in_field_size, int in_num_blocks_per_gen, int in_block_size,
			 const char *in_filename, int in_fileSize, bool dummy) {

        InitServer(in_field_size, in_num_blocks_per_gen, in_block_size, in_filename, in_fileSize, false);
	nc = new NC(field_size, is_sim);
}

CodeTorrent::CodeTorrent(int in_field_size,  int in_num_blocks_per_gen, int in_block_size,
        const char *in_filename, int in_file_size) {

	InitClient(in_field_size, in_num_blocks_per_gen, in_block_size, in_filename, in_file_size, false);
	nc = new NC(field_size, false);
}

long CT_GetTimeDifference(struct timeval tv_beg, struct timeval tv_end) {
	return ((tv_end.tv_usec - tv_beg.tv_usec) + (tv_end.tv_sec - tv_beg.tv_sec) * 1000000);
}

/*
 void gettimeofday(struct timeval, void* time_zone){
 struct _timeb cur;
 _ftime(&cur);
 tv->tv_sec =cur.time;
 tv->tv_usec = cur.millitm * 1000;
 }
 */
CodeTorrent::~CodeTorrent() {
    HAGGLE_DBG2("Calling codetorrent destructor\n");
	CleanMGData();

	if (identity == CT_SERVER) {
	    HAGGLE_DBG2("Cleaning up server\n");
	    CleanData();
	}


	if (identity == CT_CLIENT) {
	    HAGGLE_DBG2("Cleaning up client\n");
		CleanMHelpful();
		CleanBuffer();
        delete[] rank_vec;
        delete[] rank_vec_in_buf;
	}

    // CBMEN, HL - Memory leak, only allocate when needed.
	/*if (cb)
		FreeCodedBlock(cb);
    */

	delete[] num_blocks_gen;
	delete nc;
}

void CodeTorrent::CleanData() {

	int i;

	if (identity == CT_SERVER) {

		for (i = 0; i < (int) data.size(); i++) {
			FreeBlock(data[i]);
		}
		data.clear();
	}
}

void CodeTorrent::CleanBuffer() {

	if (identity == CT_CLIENT) {
	    for (std::vector<CodedBlockPtr>::iterator it = buf.begin(); it != buf.end(); ++it) {
	        FreeCodedBlock((*it));
	    }
		buf.clear();
	}
}

void CodeTorrent::CleanMGData() {

    // CBMEN, HL - We don't need this
    if (identity == CT_SERVER) {
        return;
    }

	int j;

	// free m_gdata
	for (j = 0; j < num_blocks_gen[0]; ++j) {

		delete[] m_gdata[j];
	}

	delete[] m_gdata;
}

void CodeTorrent::CleanMHelpful() {

	int i, j;

	if (m_helpful) {

		for (i = 0; i < num_gens; ++i) {
            for (j = 0; j < num_blocks_gen[i]; ++j) {
                delete[] (m_helpful[i][j]);
            }
            delete[] (m_helpful[i]);
		}

		delete[] m_helpful;
	}
}

void CodeTorrent::CleanTempFiles() {

	int gen;
	char *ext = ".temp";
	char *ext2 = new char[3]; // what if more than 100 gens?

	int fname_len = strlen(filename);
	int ext_len = strlen(ext);
	memset(ext2,0,3);
	int ext2_len = strlen(ext2);

	char *filename_read = new char[fname_len + ext_len + ext2_len + 1];

	for (gen = 0; gen < num_gens; gen++) {
#ifdef WIN32
		itoa(gen, ext2, 10); // base : 10
#else
		sprintf(ext2, "%d", gen);
#endif
		memcpy(filename_read, filename, fname_len);
		memcpy(filename_read + fname_len, ext, ext_len);
		memcpy(filename_read + fname_len + ext_len, ext2, ext2_len);

		filename_read[fname_len + ext_len + ext2_len] = '\0';

		remove(filename_read);
	}

	delete[] ext2;
	delete[] filename_read;
}

// Server initialization
void CodeTorrent::InitServer(int in_field_size, int in_num_blocks_per_gen, int in_block_size, const char *in_filename,
			     int in_file_size,  bool in_is_sim) {

	int j;

	// set internal parameters
	field_size = in_field_size;
	//num_gens = in_num_gens;				// computed in LoadFileInfo()
	block_size = in_block_size;
	strcpy(filename, in_filename);
	is_sim = in_is_sim;
	file_size = in_file_size; // MOS
	gen_completed = 0;

	// load the file info and determine num_blocks_gen
	LoadFileInfo(in_num_blocks_per_gen);

	//num_blocks_gen = new int[num_gens];	// moved to LoadFileInfo()

	// num_blocks_gen[0] must be the maximum of all num_blocks_gen[i]
    // CBMEN, HL - Only allocate when needed
	//cb = AllocCodedBlock(num_blocks_gen[0], block_size);

	identity = CT_SERVER; // I'm a server!

	//PrintFileInfo();

	// m_gdata : data for each generation
	// allocate memory to m_gdata

    /* CBMEN, HL - We don't use these ...

	m_gdata = new unsigned char*[num_blocks_gen[0]];

	for (j = 0; j < num_blocks_gen[0]; ++j) {

		m_gdata[j] = new unsigned char[(is_sim ? 1 : block_size)];
		memset(m_gdata[j], 0, block_size);
	}
    */

	gen_in_memory = -1; // no generation's loaded in memory

	file_counter = 0; //add 11/20

}

// Client
void CodeTorrent::InitClient(int in_field_size,  int in_num_blocks_per_gen, int in_block_size,
        const char *in_filename, int in_file_size, bool in_is_sim) {

	gettimeofday(&ct_tv_beg, &ct_tz);
	int i, j;



	// set internal parameters
	field_size = in_field_size;
	//num_gens = in_num_gens;
	block_size = in_block_size;
	strcpy(filename, in_filename);
	is_sim = in_is_sim;
	file_size = in_file_size;

	gen_completed = 0;

	buffer_size = in_num_blocks_per_gen * 2;
	double doubleFileSize = static_cast<double>(file_size);
	double doubleMultiplier = static_cast<double>(8);
	double doubleFieldSize = static_cast<double>(field_size);
	double doubleBlockSize = static_cast<double>(block_size);
	double doubleNumBlocks = ceil( doubleFileSize * 8 / doubleFieldSize / doubleBlockSize );
	num_blocks = static_cast<int>(doubleNumBlocks);
	//num_blocks = (int) ceil((double) file_size * 8 / field_size / block_size);

	double doubleInNumBlocksPerGen = static_cast<double>(in_num_blocks_per_gen);
	double doubleNumGens = floor( doubleNumBlocks / doubleInNumBlocksPerGen );
	num_gens = static_cast<int>(doubleNumGens);
	//num_gens = (int) floor((double) num_blocks / in_num_blocks_per_gen);

	//fprintf(stderr,"doubleFileSize=|%f| doubleMultiplier=|%f| doubleFieldSize=|%f| doubleBlockSize=|%f| "
	//		"doubleNumBlocks=|%f| num_blocks=%d doubleInNumBlocksPerGen=|%f| doubleNumGens=|%f| num_gens=%d\n",
	//		doubleFileSize,doubleMultiplier,doubleFieldSize,doubleBlockSize,doubleNumBlocks,num_blocks,
	//		doubleInNumBlocksPerGen,doubleNumGens,num_gens);

	// boundary case: generations are evenly divided
	if (in_num_blocks_per_gen * num_gens == num_blocks) {

		num_blocks_gen = new int[num_gens];

		for (i = 0; i < num_gens; ++i) {
			num_blocks_gen[i] = in_num_blocks_per_gen;
			//printf("client num_blocks_gen[i] %d\n",num_blocks_gen[i]);
		}


	} else {

		num_gens++;
		num_blocks_gen = new int[num_gens];

		for (i = 0; i < num_gens - 1; ++i) {
			num_blocks_gen[i] = in_num_blocks_per_gen;
			//printf("client num_blocks_gen[i] %d\n",num_blocks_gen[i]);
		}

		// last generation
		num_blocks_gen[num_gens - 1] = num_blocks - in_num_blocks_per_gen * (num_gens - 1);
		//printf("client num_blocks_gen[i] %d\n",num_blocks_gen[num_gens - 1]);
	}

	// num_blocks_gen[0] must be the maximum of all num_blocks_gen[i]
    // CBMEN, HL - Only allocate when needed
	//cb = AllocCodedBlock(num_blocks_gen[0], block_size);

	// allocate memory to helpfulness check matrix
	m_helpful = new unsigned char **[num_gens];

	for (i = 0; i < num_gens; ++i) {

		m_helpful[i] = new unsigned char *[num_blocks_gen[i]];

		for (j = 0; j < num_blocks_gen[i]; ++j) {
			m_helpful[i][j] = new unsigned char[(is_sim ? 1 : num_blocks_gen[i])];
			memset(m_helpful[i][j], 0, num_blocks_gen[i]);
		}
	}

	// allocate memory to rank vector and initialize them to zeros
	rank_vec = new int[num_gens];
	rank_vec_in_buf = new int[num_gens];

	for (i = 0; i < num_gens; ++i) {

		rank_vec[i] = 0;
		rank_vec_in_buf[i] = 0;

	}

	identity = CT_CLIENT; // I'm a client!

	//PrintFileInfo();

	// m_gdata : data for each generation
	// allocate memory to m_gdata
	m_gdata = new unsigned char*[num_blocks_gen[0]];

	for (j = 0; j < num_blocks_gen[0]; ++j) {

		m_gdata[j] = new unsigned char[(is_sim ? 1 : block_size)];
		memset(m_gdata[j], 0, block_size);
	}

	// output file name setup
	//char *ext = ".rec";
	//char *ext = ".received";
	char *ext = "";
	int fname_len = strlen(filename);
	int ext_len = strlen(ext);

	//out_filename = new char[fname_len + ext_len + 1];
	memcpy(out_filename, filename, fname_len);
	memcpy(out_filename + fname_len, ext, ext_len);

	out_filename[fname_len + ext_len] = '\0';

	// TODO: What should we do if the file already exists?
	remove(out_filename);

	gen_in_memory = -1; // no generation's loaded in memory

	file_counter = 0; //add 11/20
}

void CodeTorrent::LoadFileInfo(int in_num_blocks_per_gen) {

	//FILE *fp;

	int i;
	//int n_items = 0;


	/* MOS - now assumed to be set beforehand to avoid errors at this point
	FILE *file = fopen(filename, "rb");
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fclose(file);
	*/

        double doubleFileSize = static_cast<double>(file_size);
        double doubleMultiplier = static_cast<double>(8);
        double doubleFieldSize = static_cast<double>(field_size);
        double doubleBlockSize = static_cast<double>(block_size);
        double doubleNumBlocks = ceil( doubleFileSize * 8 / doubleFieldSize / doubleBlockSize );
        num_blocks = static_cast<int>(doubleNumBlocks);
	//num_blocks = (int) ceil((double) file_size * 8 / field_size / block_size);

        double doubleInNumBlocksPerGen = static_cast<double>(in_num_blocks_per_gen);
        double doubleNumGens = floor( doubleNumBlocks / doubleInNumBlocksPerGen );
        num_gens = static_cast<int>(doubleNumGens);

    	//fprintf(stderr,"filename=%s doubleFileSize=|%f| doubleMultiplier=|%f| doubleFieldSize=|%f| doubleBlockSize=|%f| "
    	//		"doubleNumBlocks=|%f| num_blocks=%d doubleInNumBlocksPerGen=|%f| doubleNumGens=|%f| num_gens=%d\n",
    	//		filename,doubleFileSize,doubleMultiplier,doubleFieldSize,doubleBlockSize,doubleNumBlocks,num_blocks,
    	//		doubleInNumBlocksPerGen,doubleNumGens,num_gens);

	//num_gens = (int) floor((double) num_blocks / in_num_blocks_per_gen);
	/**
	 * JJ Bugfix? why is num gens allowed to be 0
	 */
	if (num_gens == 0) {
		num_gens = 1;
	}

	//printf("in_num_blocks_per_gen %d\n", in_num_blocks_per_gen);
	//printf("num blocks %d\n", num_blocks);
	//printf("num gens %d\n", num_gens);

	// boundary case: generations are evenly divided
	if (in_num_blocks_per_gen * num_gens == num_blocks) {

		num_blocks_gen = new int[num_gens];

		for (i = 0; i < num_gens; ++i) {
			num_blocks_gen[i] = in_num_blocks_per_gen;
			//printf("server num_blocks_gen[i] %d\n", in_num_blocks_per_gen);
		}

	} else {

		num_gens++;
		num_blocks_gen = new int[num_gens];

		for (i = 0; i < num_gens - 1; ++i) {
			num_blocks_gen[i] = in_num_blocks_per_gen;
			//printf("server num_blocks_gen[i] %d\n", num_blocks_gen[i]);
		}

		// last generation
		num_blocks_gen[num_gens - 1] = num_blocks - in_num_blocks_per_gen * (num_gens - 1);
		//printf("server num_blocks_gen[num_gens - 1] %d\n", num_blocks_gen[num_gens - 1]);
	}
}

void CodeTorrent::LoadFile(int gen) {

	BlockPtr pblk;

	// before begining, erase everything in data
	CleanData(); // MOS - no data signals error

	//printf("loading filename=%s\n",filename);
	if (!(fp = fopen(filename, "rb"))) {
		HAGGLE_ERR("CODETORRENT ERROR: cannot open %s\n", filename);
		return; // MOS - need error handling
	}

	if(fp) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", filename, fileno(fp));

	int i, j;
	int n_items = 0;
	int pos = 0;
	int temp;
	bool last_gen = false;
	int last_block_size;

	int gen_size = block_size * num_blocks_gen[gen]; // set gen_size for this gen
	unsigned char *fbuf = new unsigned char[gen_size];

	for (i = 0; i < gen; i++)
		pos += num_blocks_gen[i];

	pos *= block_size;

	fseek(fp, pos, SEEK_SET); // move file cursor to begining of n-th generation

	// read one generation
	temp = fread(fbuf, 1, gen_size, fp);
	//printf("read one generation %s\n",fbuf);

	if (temp + pos == file_size && gen == num_gens - 1) {

		last_gen = true;

	} else if (temp != gen_size) {

	        HAGGLE_ERR("temp %d gen_size %d\n",temp,gen_size);

#ifdef WIN32
		HAGGLE_ERR("%s: fread(2) \n", strerror(errno));
#else
		HAGGLE_ERR("CODETORRENT ERROR: unexpected codetorrent read result\n");
#endif
		//printf("Press <enter>...");
		//getchar();
		//abort();

		fclose(fp); // MOS
		delete[] fbuf; // MOS
		return; // MOS - need error handling
	}

	fclose(fp);

	// before begining, erase everything in data
	// CleanData(); // MOS - see above

	int numblockslen = num_blocks_gen[gen];
	//printf("numblockslen =%d\n", numblockslen);
	// N.B. data is stored "unpacked" e.g., 4bit symbol => 8bit.
	for (i = 0; i < numblockslen; i++) {

		pblk = AllocBlock((block_size));
		memset(pblk, 0, (block_size));

		// if this is the last block
		if (last_gen && i == num_blocks_gen[gen] - 1) {

			last_block_size = temp - (num_blocks_gen[gen] - 1) * block_size;

			for (j = 0; j < (is_sim ? 1 : last_block_size); j++) {
				pblk[j] = NthSymbol(fbuf, field_size, block_size * i + j);
			}

			if (!is_sim) {

				for (; j < block_size; j++) {
					pblk[j] = 0; // padding zeros
				}
			}
		} else {

			for (j = 0; j < (is_sim ? 1 : block_size); j++) {
				pblk[j] = NthSymbol(fbuf, field_size, block_size * i + j);
			}
		}

		//printf("pushing back block |%u|\n", pblk);
		data.push_back(pblk);
	}

	// record which gen is in memory!
	gen_in_memory = gen;

	delete[] fbuf;
}

unsigned char CodeTorrent::NthSymbol(unsigned char *in_buf, int fsize, int at) {

	unsigned char sym;

	// field size must be 2^x
	assert( fsize%2 == 0);
	//fprintf(stderr,"fsize=%d at=%d",fsize,at);
	// total number of symbols per CHAR.
	unsigned int numsyms = (unsigned int) 8 / fsize;
	sym = in_buf[(int) at / numsyms];
	sym >>= ((numsyms - at % numsyms - 1) * fsize);
	sym &= (((unsigned char) -1) >> (8 - fsize));

	return sym;
}

// This function is obsolete
/*
 void CodeTorrent::WriteFile() {

 char *ext = ".rec";
 int fname_len = strlen(filename);
 int ext_len = strlen(ext);

 char *filename_write = new char[fname_len + ext_len + 1];
 memcpy(filename_write, filename, fname_len);
 memcpy(filename_write+fname_len, ext, ext_len);

 filename_write[fname_len+ext_len] = '\0';

 FILE *fp_write;

 if( !(fp_write = fopen(filename_write, "wb")) ) {
 fprintf(stderr, "Cannot write to $s.\n", filename_write);
 exit(1);	// TODO: instead of exiting, we should ...
 }

 int i,j,k;
 int pos=0;

 for( i=0 ; i < num_gens ; i++ ) {

 for( j=0 ; j < num_blocks_gen[i] ; j++ ) {

 if( i == num_gens-1 && j == num_blocks_gen[i]-1 ) {

 for( k=0 ; k < num_gens-1 ; k++ ) {
 pos += num_blocks_gen[k];
 }

 int last_block_size = file_size - (pos+num_blocks_gen[i]-1)*block_size;

 fwrite(m_data[i][j], 1, last_block_size, fp_write);

 } else {

 fwrite(m_data[i][j], 1, block_size, fp_write);
 }
 }
 }

 fclose(fp_write);
 }
 */

bool CodeTorrent::WriteFile(int gen) {
    HAGGLE_DBG2("Writing received data to file %s\n",out_filename);
	fp_write = fopen(out_filename, "wb");
	if(fp_write == NULL) {
	  HAGGLE_ERR("Cannot create file %s for decoded data\n",out_filename);
	  return false;
	}

    if(fp_write) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", out_filename, fileno(fp_write));

	//fseek(fp_write, 1, SEEK_END);

	int j, k;
	int pos = 0;

	for (j = 0; j < num_blocks_gen[gen]; j++) {

		//printf("data=%s num_blocks_gen[gen]=%d\n",m_gdata[j],num_blocks_gen[gen]);

		if (gen == num_gens - 1 && j == num_blocks_gen[gen] - 1) {

			for (k = 0; k < num_gens - 1; k++) {
				pos += num_blocks_gen[k];
			}

			int last_block_size = file_size - (pos + num_blocks_gen[gen] - 1) * block_size;


			int written = fwrite(m_gdata[j], 1, last_block_size, fp_write);
			if(written != last_block_size) {
			  HAGGLE_ERR("Error-1 writing decoded data to file %s\n",out_filename); // MOS
			  fclose(fp_write);
			  return false;
			}
		} else {
			int written = fwrite(m_gdata[j], 1, block_size, fp_write);
			if(written != block_size) {
			  HAGGLE_ERR("Error-2 writing decoded data to file %s\n",out_filename); // MOS
			  fclose(fp_write);
			  return false;
			}
		}
	}

	fclose(fp_write);
	return true;
}

BlockPtr CodeTorrent::AllocBlock(int in_block_size) {

	return (BlockPtr) malloc(in_block_size);
}

void CodeTorrent::FreeBlock(BlockPtr blk) {

	free(blk);
}

CoeffsPtr CodeTorrent::AllocCoeffs(int numblks) {
	return (CoeffsPtr) malloc(numblks);
}

void CodeTorrent::FreeCoeffs(CoeffsPtr blk) {
	free(blk);
}

CodedBlockPtr CodeTorrent::AllocCodedBlock(int numblks, int blksize) {
	CodedBlock *blk = NULL;

	//printf("numblks %d blksize %d\n", numblks, blksize);
	blk = (CodedBlock *) malloc(sizeof(CodedBlock));
	assert(blk);

	blk->coeffs = AllocCoeffs(numblks);
	blk->sums = AllocBlock(blksize);
	blk->block_size = blksize;
	blk->num_blocks_gen = numblks;

	assert(blk->coeffs);
	assert(blk->sums);

	return blk;
}

void CodeTorrent::FreeCodedBlock(CodedBlockPtr ptr) {
	FreeCoeffs(ptr->coeffs);
	FreeBlock(ptr->sums);
	free(ptr);
}

CodedBlock* CodeTorrent::CopyCodedBlock(CodedBlockPtr ptr) {
	CodedBlock *new_blk = AllocCodedBlock(ptr->num_blocks_gen, ptr->block_size);
	new_blk->gen = ptr->gen;
	new_blk->num_blocks_gen = ptr->num_blocks_gen;
	new_blk->block_size = ptr->block_size;

	memcpy(new_blk->coeffs, ptr->coeffs, new_blk->num_blocks_gen);
	memcpy(new_blk->sums, ptr->sums, new_blk->block_size);

	return new_blk;
}

/*BlockPtr CodeTorrent::LoadSingleBlock(int gen, int blockIdInGen){
	LoadFile(gen);
	return data[blockIdInGen];
}*/


// encode a block from generation "gen"
/*CodedBlockPtr CodeTorrent::EncodeSingleBlock(int gen, int blockIdInGen, BlockPtr fragBlockData, int fragDataSize) {

	if(fragDataSize != block_size){
		//TODO: Report error
//		std::cerr<<"fragment size != block size"<<std::endl;
		return NULL;
	}

	// create a new copy
	CodedBlockPtr cb_to = AllocCodedBlock(num_blocks_gen[gen], block_size);

	cb_to->gen = gen;
	cb_to->num_blocks_gen = num_blocks_gen[gen];
	cb_to->block_size = block_size;

	//Make a fake data to encode
	std::vector<BlockPtr> fakeData;
	BlockPtr pblk;
	for(int i=0;i<num_blocks_gen[gen];i++){
		pblk = AllocBlock((block_size));
		memset(pblk, 0, (block_size));
		if(i==blockIdInGen){
			memcpy(pblk, fragBlockData, (block_size));
		}
		fakeData.push_back(pblk);
	}

	nc->EncodeSingleBlock(fakeData, cb_to, blockIdInGen);

	//release fake data
	for(int i=0;i<(int) fakeData.size();i++){
		FreeBlock(fakeData[i]);
	}
	fakeData.clear();

	return cb_to;
}*/

// encode a block from generation "gen"
CodedBlockPtr CodeTorrent::Encode(int gen) {

	// if the data's not already in memory, load it
	if (gen_in_memory != gen) {
		LoadFile(gen);
		if(data.size() == 0) return NULL; // MOS
	}

	// create a new copy
	CodedBlockPtr cb_to = AllocCodedBlock(num_blocks_gen[gen], block_size);

	cb_to->gen = gen;
	cb_to->num_blocks_gen = num_blocks_gen[gen];
	cb_to->block_size = block_size;

	gettimeofday(&ct_encode_beg, &ct_tz);
	nc->EncodeBlock(data, cb_to);
	gettimeofday(&ct_encode_end, &ct_tz);
//	printf("[CT_Encode] Encode time: %ld.%03ld\n", CT_GetTimeDifference(ct_encode_beg, ct_encode_end) / 1000,
//			CT_GetTimeDifference(ct_encode_beg, ct_encode_end) % 1000);

	return cb_to;
}

// re-encode a block from generation "gen"
// return NULL pointer if failed (e.g. no data in buffer)
CodedBlockPtr CodeTorrent::ReEncode(int gen) {

	int j;

	// TODO: Make this work with file buffers
    // CBMEN, HL - Allocate memory here
    CodedBlockPtr cb = AllocCodedBlock(num_blocks_gen[gen], block_size);
    if (!cb)
        return NULL;
	cb->gen = gen;
	cb->num_blocks_gen = num_blocks_gen[gen];
	cb->block_size = block_size;

	//std::vector<CodedBlockPtr> *tempBuf = new std::vector<CodedBlockPtr>[GetRankVec()[gen]];
	std::vector<CodedBlockPtr> tempBuf;

	gettimeofday(&ct_vector_beg, &ct_tz);

	for (std::vector<CodedBlockPtr>::iterator it = buf.begin(); it != buf.end(); ++it) {
		if ((*it)->gen == gen)
			tempBuf.push_back(CopyCodedBlock((*it)));
	}

	int blocks_in_file = rank_vec[gen] - rank_vec_in_buf[gen];

	for (j = 0; j < blocks_in_file; j++) {
		tempBuf.push_back(ReadGenBuf(gen, j));
	}

	gettimeofday(&ct_vector_end, &ct_tz);
	//printf("[CT_Encode] Vector time: %ld.%03ld\n", CT_GetTimeDifference(ct_vector_beg, ct_vector_end) / 1000,
	//		CT_GetTimeDifference(ct_vector_beg, ct_vector_end) % 1000);

	//gettimeofday(&ct_encode_beg, &ct_tz);

	if (!nc->ReEncodeBlock(tempBuf, cb)) {
		return NULL;
	}

	//gettimeofday(&ct_encode_end, &ct_tz);
	//printf("[CT_Encode] Encode time: %ld.%03ld\n", CT_GetTimeDifference(ct_encode_beg, ct_encode_end)/1000, CT_GetTimeDifference(ct_encode_beg, ct_encode_end)%1000);

	for (j = 0; j < tempBuf.size(); j++)
		FreeCodedBlock(tempBuf[j]);
	tempBuf.clear();

    // CBMEN, HL - no need for redundant copy
    return cb;
	//return CopyCodedBlock(cb);
}

// store a new block into the buffer
bool CodeTorrent::StoreBlock(CodedBlockPtr in) {

	int gen = in->gen;

	if (nc->IsHelpful(rank_vec, m_helpful, in)) {
		HAGGLE_DBG2("Block is helpful\n");
		buf.push_back(CopyCodedBlock(in));
		rank_vec[gen]++; // if helpful, raise the rank
		rank_vec_in_buf[gen]++;

		if (buf.size() >= buffer_size) { // if the buf goes above the threshold
			FlushBuf();
		}

	} else {
	    HAGGLE_DBG("Block is not helpful\n");
		return false; // if not helpful, return false
	}

	// if full-rank, this generation is complete
	if (rank_vec[gen] == GetNumBlocksGen(gen)) {

		IncrementGenCompleted();
	}

	return true;
}

// decode!
bool CodeTorrent::Decode() {

	int i;
	//struct timeval start_dec, end_dec;

	if (identity != CT_CLIENT) {// if I'm a server, why do it?
	    HAGGLE_ERR("Server is trying to decode\n");
		return false;
	}

	//why in the world does the identity get reset to server?
	//identity = CT_SERVER;

	if (GetGenCompleted() != GetNumGens()) { // make sure all generations are in!

	    HAGGLE_DBG("The file is not complete, decoding failed - GenCompleted=%d NumGens=%d\n",GetGenCompleted(),GetNumGens());
		//fprintf(stderr, "The file download is not complete, decoding failed.\n");

		return false;
	}

	//gettimeofday(&start_dec, &ct_tz);
	//printf("Decoding: ");

	for (i = 0; i < GetNumGens(); i++) {

		//printf("%d  :  ", i);
	        if(!DecodeGen(i)) {
		  HAGGLE_ERR("Decoding failure\n"); // MOS
		  return false;
	        }
	}
	/*
	 gettimeofday(&end_dec, &ct_tz);
	 printf("Decoding time:  %ld.%ld\n", CT_GetTimeDifference(start_dec, end_dec)/1000,
	 CT_GetTimeDifference(start_dec, end_dec)%1000);
	 */
	//printf("done!\n");
	//gettimeofday(&ct_tv_end, &ct_tz);
	//printf("TIME!!! %ld.%ld\n", CT_GetTimeDifference(ct_tv_beg, ct_tv_end)/1000, CT_GetTimeDifference(ct_tv_beg, ct_tv_end)%1000);

	// we don't need this stuff anymore
	CleanBuffer();
	CleanTempFiles();

	return true;
}

bool CodeTorrent::DecodeGen(int gen) {
    HAGGLE_DBG2("Performing decoding\n");
	int j;

	// allocate m_data
	//m_gdata = new unsigned char *[num_gens];

	std::vector<CodedBlockPtr> tempBuf; // buffer to save encoded_blocks from file and current buffer

	// fill in buffer! from file and current buffer
	//for( j=0 ; j <  buf.size() ; j++ ) {
	for (std::vector<CodedBlockPtr>::iterator it = buf.begin(); it != buf.end(); ++it) {
		if ((*it)->gen == gen)
			tempBuf.push_back(CopyCodedBlock((*it)));
	}

	int blocks_in_file = rank_vec[gen] - rank_vec_in_buf[gen];

	for (j = 0; j < blocks_in_file; j++) {
		tempBuf.push_back(ReadGenBuf(gen, j));
	}

	bool isDecode = nc->Decode(tempBuf, m_gdata, gen);
	if(!isDecode) {
	    HAGGLE_ERR("Decoding not complete\n");
		return false;
	}

	// when done decoding, write it into a file
	if(!WriteFile(gen)) {
	    HAGGLE_ERR("Error writing decoded data to file\n"); // MOS
	    return false;
	}

	// free tempBuf
	size_t loopCounter = 0;
	for (loopCounter = 0; loopCounter < tempBuf.size(); loopCounter++)
		FreeCodedBlock(tempBuf[loopCounter]);
	tempBuf.clear();

	return true;
}

// each file buffer stores an array of coded blocks
// +-------+------------+-------+------------+-----
// | coeff | coded data | coeff | coded data | ...
// +-------+------------+-------+------------+-----
void CodeTorrent::PushGenBuf(CodedBlockPtr in) {

	int gen = in->gen;

	const char *ext = ".temp";
	char *ext2 = new char[3]; // what if more than 100 gens?
#ifdef WIN32
	itoa(gen, ext2, 10); // files are to be named "~~~.temp0", "~~~.temp1", etc.
#else
	sprintf(ext2, "%d", gen);
#endif

	int fname_len = strlen(filename);
	int ext_len = strlen(ext);
	int ext2_len = strlen(ext2);

	// this is dumb but works
	char *filename_write = new char[fname_len + ext_len + ext2_len + 1];

	memcpy(filename_write, filename, fname_len);
	memcpy(filename_write + fname_len, ext, ext_len);
	memcpy(filename_write + fname_len + ext_len, ext2, ext2_len);

	filename_write[fname_len + ext_len + ext2_len] = '\0';

	fp_write = fopen(filename_write, "ab"); // append to the file
	if(fp_write) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", filename_write, fileno(fp_write));
											// TODO: error checking?
	int cf = fwrite(in->coeffs, 1, num_blocks_gen[gen], fp_write); // write the coeffs first
	int sm = fwrite(in->sums, 1, block_size, fp_write); // write actual coded block

	if (cf != num_blocks_gen[gen] || sm != block_size) {
		HAGGLE_ERR("cache writing error!\n");
	}

	delete[] ext2;
	delete[] filename_write;

	fclose(fp_write); // close the file
}

// each file buffer stores an array of coded blocks
// +-------+------------+-------+------------+-----
// | coeff | coded data | coeff | coded data | ...
// +-------+------------+-------+------------+-----
CodedBlockPtr CodeTorrent::ReadGenBuf(int gen, int k) {

	// NOTE: error checking must be done prior to invoking this method!
	//		(i.e. are there strictly less than k blocks in this file buffer?)
	// NOTE: k begins at 0

	CodedBlockPtr tempBlock;
	tempBlock = AllocCodedBlock(num_blocks_gen[gen], block_size);
	tempBlock->gen = gen;

	const char *ext = ".temp";
	char *ext2 = new char[3]; // what if more than 100 gens?

#ifdef WIN32
	itoa(gen, ext2, 10); // base : 10
#else
	sprintf(ext2, "%d", gen);
#endif

	int fname_len = strlen(filename);
	int ext_len = strlen(ext);
	int ext2_len = strlen(ext2);

	// this is dumb but works
	char *filename_read = new char[fname_len + ext_len + ext2_len + 1];

	memcpy(filename_read, filename, fname_len);
	memcpy(filename_read + fname_len, ext, ext_len);
	memcpy(filename_read + fname_len + ext_len, ext2, ext2_len);

	filename_read[fname_len + ext_len + ext2_len] = '\0';

	fp = fopen(filename_read, "rb");
	if(fp) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", filename_read, fileno(fp));

	if (!fp) {
		HAGGLE_ERR("CODETORRENT ERROR: cache access error!\n");
	}

	if (fseek(fp, (num_blocks_gen[gen] + block_size) * k, SEEK_SET)) {
		HAGGLE_ERR("CODETORRENT ERROR: cache access error!\n");
	}

	int cf = fread(tempBlock->coeffs, 1, num_blocks_gen[gen], fp);
	int sm = fread(tempBlock->sums, 1, block_size, fp);

	if (cf != num_blocks_gen[gen] || sm != block_size) {
		HAGGLE_ERR("CODETORRENT ERROR: cache reading error!\n");
	}

	fclose(fp);

	delete[] filename_read;
	delete[] ext2;

	return tempBlock;
}

void CodeTorrent::FlushBuf() {

	//pthread_mutex_lock(&work_mutex);

	if (!(buf.size() >= buffer_size)) { // Should I really flush it?
		//
		return;
	}

	// NOTE: For now, we select a random generation and evict it from the buffer
	int i;

	int gen_to_evict = rand() % num_gens;

	CodedBlockPtr tempBlock;

	while (rank_vec_in_buf[gen_to_evict] <= 0) {
		gen_to_evict = rand() % num_gens;
	}

	for (i = buf.size() - 1; i >= 0; i--) {

		tempBlock = buf[i];

		if (tempBlock->gen == gen_to_evict) { // if this block is from the selected generation

			PushGenBuf(tempBlock); // push it to the file buffer
			FreeCodedBlock(tempBlock); // and remove it from memory buffer
			buf.erase(buf.begin() + i);
			rank_vec_in_buf[gen_to_evict]--; // decrement rank of gen i in buf
		}

		if (rank_vec_in_buf[gen_to_evict] <= 0)
			break;
	}

	//
}

void CodeTorrent::PrintBlocks(int gen) {
	int j, k;

	HAGGLE_DBG("===== Original Data ====\n");

	for (j = 0; j < num_blocks_gen[gen]; j++) {

		HAGGLE_DBG("[Gen#%2u Blk#%3u] ", gen, j); // gen ID and block ID.

		for (k = 0; k < ((is_sim ? 1 : block_size) > PRINT_BLK ? PRINT_BLK : (is_sim ? 1 : block_size)); k++) {
			HAGGLE_DBG("%x ", data[j][k]); // j-th block, k-th character
		}

		if ((is_sim ? 1 : block_size) > PRINT_BLK * 2) {
			HAGGLE_DBG(" ... ");
		}

		for (k = (is_sim ? 1 : block_size) - PRINT_BLK; k < (is_sim ? 1 : block_size); k++) {
			HAGGLE_DBG("%x ", data[j][k]);
		}

		HAGGLE_DBG("\n");

	}
	HAGGLE_DBG("=======================\n");

}

// print-out decoded data (member "m_d")
void CodeTorrent::PrintDecoded() {
	/*
	 int i,j,k;

	 printf("===== Decoded Data =====\n");

	 for( i=0 ; i < num_gens ; i++ ) {

	 for ( j=0 ; j < num_blocks_gen[i] ; j++ ) {

	 printf("[Gen#%2u Blk#%3u] ", i, j); // block ID.

	 for ( k=0 ; k < ((is_sim ? 1 : block_size) > PRINT_BLK ? PRINT_BLK : (is_sim ? 1 : block_size)) ; k++ ) {

	 printf("%x ", (char)m_data[i][j][k]);
	 }

	 if( (is_sim?1:block_size) > PRINT_BLK*2 ) {

	 printf(" ... ");

	 for (k=(is_sim ? 1 : block_size)-PRINT_BLK; k < (is_sim? 1 : block_size); k++ ) {

	 printf("%x ", (char)m_data[i][j][k]);
	 }
	 }
	 printf("\n");
	 }
	 }
	 printf("=======================\n");
	 */

}

//added
/*
 void CodeTorrent::Update(int in_file_id, int in_num_blocks_per_gen, int in_block_size, char *in_filename, int in_file_size)
 {
 file_id[0] = in_file_id; //??
 num_blocks_per_gen = in_num_blocks_per_gen;
 block_size = in_block_size;
 strcpy(filename, in_finename);
 file_size = in_file_size;
 }
 */

void CodeTorrent::PrintFileInfo() {

	HAGGLE_DBG(" ========= File Info =========\n");
	if (identity == CT_SERVER)
		HAGGLE_DBG(" SERVER\n");
	else
		HAGGLE_DBG(" CLIENT\n");
	HAGGLE_DBG(" File name: %s\n", GetFileName());
	HAGGLE_DBG(" File Size: %d\n", GetFileSize());
	HAGGLE_DBG(" Number Generations: %d\n", GetNumGens());
	HAGGLE_DBG(" Block Size: %d\n", GetBlockSize());
	HAGGLE_DBG(" Num Blocks: %d\n", GetNumBlocks());
	//HAGGLE_DBG(" Num Blocks per Gen: %d\n", GetNumBlocksGen());
	HAGGLE_DBG(" Field Size: %d bits\n", GetFieldSize());
	HAGGLE_DBG(" ============================\n");
}
