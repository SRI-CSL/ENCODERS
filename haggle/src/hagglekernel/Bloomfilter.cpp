/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Bloomfilter.h"
#include <haggleutils.h>
#include "Debug.h"
#include "Trace.h"
#include "XMLMetadata.h"

#include "networkcoding/bloomfilter/NetworkCodingBloomFilterHelper.h"
#include "fragmentation/bloomfilter/FragmentationBloomFilterHelper.h"

double Bloomfilter::default_error_rate = DEFAULT_BLOOMFILTER_ERROR_RATE;
unsigned int Bloomfilter::default_capacity = DEFAULT_BLOOMFILTER_CAPACITY;

const char *Bloomfilter::type_str[] = {
	"normal",
	"counting",
	"undefined",
	NULL
};

Bloomfilter::Type_t Bloomfilter::strToType(const char *str)
{
	int i = 0;
	
	if (!str)
		return TYPE_UNDEFINED;
	
	while (type_str[i]) {
		if (strcmp(str, type_str[i]) == 0)
			return (Type_t)i;
	
		i++;
	}
	
	return TYPE_UNDEFINED;
}

Bloomfilter::Bloomfilter(Type_t _type, double _error_rate, unsigned int _capacity) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_BLOOMFILTER),
#endif
	type(_type),
	error_rate(_error_rate),
	capacity(_capacity),
	init_n(0),
	raw(NULL),
	updates(0)
{
	if (type == TYPE_COUNTING) {
		cbf = counting_bloomfilter_new(error_rate, capacity);
	} else {
		bf = bloomfilter_new(error_rate, capacity);
	}
	this->networkCodingConfiguration = new NetworkCodingConfiguration();
	this->networkCodingBloomFilterInterface = new NetworkCodingBloomFilterHelper();
	this->fragmentationBloomFilterInterface = new FragmentationBloomFilterHelper();
	this->fragmentationConfiguration = new FragmentationConfiguration();
}

Bloomfilter::Bloomfilter(double _error_rate, unsigned int _capacity, struct bloomfilter *_bf) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_BLOOMFILTER),
#endif
	type(TYPE_NORMAL),
	error_rate(_error_rate),
	capacity(_capacity),
	init_n(_bf->n),
	bf(_bf),
	updates(0)
{
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
    this->networkCodingBloomFilterInterface = new NetworkCodingBloomFilterHelper();
    this->fragmentationBloomFilterInterface = new FragmentationBloomFilterHelper();
    this->fragmentationConfiguration = new FragmentationConfiguration();
}

Bloomfilter::Bloomfilter(double _error_rate, unsigned int _capacity, struct counting_bloomfilter *_cbf) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_BLOOMFILTER),
#endif
	type(TYPE_COUNTING),
	error_rate(_error_rate),
	capacity(_capacity),
	init_n(_cbf->n),
	cbf(_cbf),
	updates(0)
{
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
    this->networkCodingBloomFilterInterface = new NetworkCodingBloomFilterHelper();
    this->fragmentationBloomFilterInterface = new FragmentationBloomFilterHelper();
    this->fragmentationConfiguration = new FragmentationConfiguration();
}

Bloomfilter::Bloomfilter(const Bloomfilter &_bf) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_BLOOMFILTER),
#endif
	type(_bf.type),
	error_rate(_bf.error_rate),
	capacity(_bf.capacity),
	init_n(_bf.init_n),
	raw(NULL),
	updates(0)
{
	if (type == TYPE_NORMAL) {
		bf = bloomfilter_copy(_bf.bf);
	} else {
		cbf = counting_bloomfilter_copy(_bf.cbf);
	} 
	this->networkCodingConfiguration = new NetworkCodingConfiguration();
	this->networkCodingBloomFilterInterface = new NetworkCodingBloomFilterHelper();
	this->fragmentationBloomFilterInterface = new FragmentationBloomFilterHelper();
	this->fragmentationConfiguration = new FragmentationConfiguration();
}

Bloomfilter *Bloomfilter::create(double error_rate, unsigned int capacity, struct bloomfilter *bf)
{
	return new Bloomfilter(error_rate, capacity, bf);
}

Bloomfilter *Bloomfilter::create(double error_rate, unsigned int capacity, struct counting_bloomfilter *cbf)
{
	return new Bloomfilter(error_rate, capacity, cbf);
}

/**
   Creates a bloomfilter with the given error rate and capacity.
*/
Bloomfilter *Bloomfilter::create(Type_t type, double error_rate, unsigned int capacity)
{
	Bloomfilter *bf = new Bloomfilter(type, error_rate, capacity);
	
	if (!bf)
		return NULL;

	if (!bf->raw) {
		delete bf;
		return NULL;
	}

	return bf;
}

Bloomfilter *Bloomfilter::create(const unsigned char *raw_bf, size_t len)
{
	Bloomfilter *bf = NULL;

	if (BLOOMFILTER_TOT_LEN((struct bloomfilter *)raw_bf) == len) {
		struct bloomfilter *c_bf = bloomfilter_copy((struct bloomfilter*)raw_bf);
		
		if (!c_bf)
			return NULL;
		
		bf = create(default_error_rate, default_capacity, c_bf);
		
		if (!bf) {
			bloomfilter_free(c_bf);
			return NULL;
		}
		HAGGLE_DBG("Bloomfilter is non-counting and contains %lu objects\n", bloomfilter_get_n(bf->bf)); 
	} else if (COUNTING_BLOOMFILTER_TOT_LEN((struct counting_bloomfilter *)raw_bf) == len){
		struct counting_bloomfilter *c_cbf = counting_bloomfilter_copy((struct counting_bloomfilter*)raw_bf);
		
		if (!c_cbf)
			return NULL;

		bf = create(default_error_rate, default_capacity, c_cbf);
		
		if (!bf) {
			counting_bloomfilter_free(c_cbf);
			return NULL;
		}
		
		HAGGLE_DBG("Bloomfilter is counting and contains %lu objects\n", counting_bloomfilter_get_n(bf->cbf)); 
	} else {
		HAGGLE_ERR("bloomfilter is neither counting nor non-counting\n");
		return NULL;
	}

	return bf;
}

Bloomfilter *Bloomfilter::create_from_base64(Type_t type, const string& b64, double _error_rate, unsigned int _capacity)
{
	Bloomfilter *bloomfilter = NULL;

	if (type == TYPE_NORMAL) {
		struct bloomfilter *bf = base64_to_bloomfilter(b64.c_str(), b64.length());
		if (bf) {
			bloomfilter = new Bloomfilter(_error_rate, _capacity, bf);		
		} else {
			HAGGLE_ERR("Bloomfilter assignment failed!\n");
		}

	} else {
		struct counting_bloomfilter *cbf = base64_to_counting_bloomfilter(b64.c_str(), b64.length());

		if (cbf) {
			bloomfilter = new Bloomfilter(_error_rate, _capacity, cbf);		
		} else {
			HAGGLE_ERR("Bloomfilter (counting) assignment failed!\n");
		}
	} 

	return bloomfilter;
}

Bloomfilter *Bloomfilter::create(const Bloomfilter &bf)
{
	Bloomfilter *bf_copy = new Bloomfilter(bf);
	
	if (!bf_copy)
		return NULL;

	if (!bf_copy->raw) {
		delete bf_copy;
		return NULL;
	}
	return bf_copy;
}

Bloomfilter::~Bloomfilter()
{
    if(this->networkCodingConfiguration) {
        delete this->networkCodingConfiguration;
        this->networkCodingConfiguration = NULL;
    }
    if(this->networkCodingBloomFilterInterface) {
        delete this->networkCodingBloomFilterInterface;
        this->networkCodingBloomFilterInterface = NULL;
    }
    if(this->fragmentationBloomFilterInterface) {
    	delete this->fragmentationBloomFilterInterface;
    	this->fragmentationBloomFilterInterface = NULL;
    }
    if(this->fragmentationConfiguration) {
    	delete this->fragmentationConfiguration;
    	this->fragmentationConfiguration = NULL;
    }

	if (raw) {
		if (type == TYPE_NORMAL)
			bloomfilter_free(bf);
		else
			counting_bloomfilter_free(cbf);
	}
}
bool Bloomfilter::add(const unsigned char *blob, size_t len)
{
	int ret = 0;

	if (type == TYPE_NORMAL) {
		ret = bloomfilter_add(bf, (const char *)blob, len);
	} else {
		ret = counting_bloomfilter_add(cbf, (const char *)blob, len);
	}

	if(ret) updates++;
	return ret == 1;
}

bool Bloomfilter::add(const DataObjectId_t& id)
{
	return add(id, DATAOBJECT_ID_LEN);
}

bool Bloomfilter::add(const DataObjectRef &dObj)
{
	return add(dObj->getId());
}

bool Bloomfilter::addParentDataObject(const DataObjectRef &dataObject) {
    bool isNetworkCodingEnabled = this->networkCodingConfiguration->isNetworkCodingEnabled(dataObject,NULL);
    bool isFragmentationEnabled = this->fragmentationConfiguration->isFragmentationEnabled(dataObject,NULL);

    if(isNetworkCodingEnabled) {
    	DataObjectIdRef dataObjectIdToLookup = this->networkCodingBloomFilterInterface->getOriginalDataObjectId(dataObject);
        if(!dataObjectIdToLookup.empty()) {
          const unsigned char * dataObjectIdPtr = dataObjectIdToLookup.c_str();
          bool ret = this->add(dataObjectIdPtr,DATAOBJECT_ID_LEN);
          HAGGLE_DBG2("adding parent object %s of networkcoded block\n",dataObject->getIdStr());
          return ret;
        }
    }
    else if(isFragmentationEnabled) {
    	DataObjectIdRef dataObjectIdToLookup = this->fragmentationBloomFilterInterface->getOriginalDataObjectId(dataObject);
        if(!dataObjectIdToLookup.empty()) {
          const unsigned char * dataObjectIdPtr = dataObjectIdToLookup.c_str();
          bool ret = this->add(dataObjectIdPtr,DATAOBJECT_ID_LEN);
          HAGGLE_DBG2("adding parent data object %s of fragment\n",dataObject->getIdStr());
          return ret;
        }
    }


    HAGGLE_DBG("Cannot find parent of data object %s\n",dataObject->getIdStr());
    return false;
}

bool Bloomfilter::remove(const unsigned char *blob, size_t len)
{
	int ret = 0;

	if (type == TYPE_COUNTING) {
		ret = counting_bloomfilter_remove(cbf, (const char *)blob, len);
	} else {
		HAGGLE_ERR("Cannot remove object from non counting bloomfilter\n");
	}

	if(ret) updates++;
	return ret == 1;
}

bool Bloomfilter::remove(const DataObjectId_t& id)
{
	return remove(id, DATAOBJECT_ID_LEN);
}

bool Bloomfilter::remove(const DataObjectRef &dObj)
{
	return remove(dObj->getId());
}

bool Bloomfilter::has(const unsigned char *blob, size_t len) const
{
	if (type == TYPE_NORMAL) {
		return bloomfilter_check(bf, (const char *)blob, len) == 1;
	} else {
		return counting_bloomfilter_check(cbf, (const char *)blob, len) == 1;
	}
}

bool Bloomfilter::has(const DataObjectId_t& id) const
{
	return has(id, DATAOBJECT_ID_LEN);
}

bool Bloomfilter::has(const DataObjectRef &dObj) const
{
	return has(dObj->getId());
}

bool Bloomfilter::hasNetworkCodingParentDataObject(const DataObjectRef &dObj) const {
    bool isNetworkCodingEnabled = this->networkCodingConfiguration->isNetworkCodingEnabled(dObj,NULL);

    if( isNetworkCodingEnabled && this->networkCodingBloomFilterInterface->isNetworkCodedObject(dObj) ) {

        DataObjectIdRef dataObjectIdRef = this->networkCodingBloomFilterInterface->getOriginalDataObjectId(dObj);
        bool hasParentDataObject = false;

        if(!dataObjectIdRef.empty()) {
            const unsigned char * dataObjectIdPtr = dataObjectIdRef.c_str();
            hasParentDataObject =  this->has(dataObjectIdPtr,DATAOBJECT_ID_LEN);
        }

	if(hasParentDataObject)
	  HAGGLE_DBG2("parent data object of block %s is already in Bloomfilter\n",dObj->getIdStr());

        if(hasParentDataObject) return true;
    }
    return false;
}

bool Bloomfilter::hasFragmentationParentDataObject(const DataObjectRef &dObj) const {
    bool isFragmentationEnabled = this->fragmentationConfiguration->isFragmentationEnabled(dObj,NULL);

    if(isFragmentationEnabled && (this->networkCodingBloomFilterInterface->isNetworkCodedObject(dObj) ||
				  this->fragmentationBloomFilterInterface->isFragmentationObject(dObj))) {

        DataObjectIdRef dataObjectIdRef = this->fragmentationBloomFilterInterface->getOriginalDataObjectId(dObj);
        bool hasParentDataObject = false;

        if(!dataObjectIdRef.empty()) {
            const unsigned char * dataObjectIdPtr = dataObjectIdRef.c_str();
            hasParentDataObject =  this->has(dataObjectIdPtr,DATAOBJECT_ID_LEN);
        }

	if(hasParentDataObject)
	  HAGGLE_DBG2("parent data object of fragment %s is already in Bloomfilter\n",dObj->getIdStr());

        if(hasParentDataObject) return true;
    }
    return false;
}

bool Bloomfilter::hasParentDataObject(const DataObjectRef &dObj) const {
  return hasNetworkCodingParentDataObject(dObj) || hasFragmentationParentDataObject(dObj);
}

bool Bloomfilter::hasThisOrParentDataObject(const DataObjectRef &dObj) const {
    bool hasOriginalDataObject = this->has(dObj->getId());
    if(hasOriginalDataObject) return true; // fastpath?

    bool hasParentDataObject = this->hasParentDataObject(dObj);

    if(hasParentDataObject) {
    	return hasParentDataObject;
    }
    else {
        return hasOriginalDataObject;
    }
}

bool Bloomfilter::merge(const Bloomfilter& bf_merge)
{
	if (type == TYPE_NORMAL && bf_merge.type == TYPE_NORMAL){
		// Cannot merge a counting bloomfilter
		if (BLOOMFILTER_TOT_LEN(bf) != BLOOMFILTER_TOT_LEN(bf_merge.bf)) {
			HAGGLE_ERR("Cannot merge bloomfilters of different size\n");
			return false;
		}
		bool res = bloomfilter_merge(bf, bf_merge.bf) == MERGE_RESULT_OK;
		
		bf->n -= init_n;
		if(res) updates++;
		return res;
	} 
	return false;
}

Bloomfilter *Bloomfilter::to_noncounting() const
{
	struct bloomfilter *bf_copy;

	if (type == TYPE_NORMAL) {
		return new Bloomfilter(*this);	
	} 

	bf_copy = counting_bloomfilter_to_noncounting(cbf);

	if (!bf_copy)
		return NULL;

	return new Bloomfilter(error_rate, capacity, bf_copy);
}

string Bloomfilter::toBase64(void) const
{
	string retval;
	
	if (type == TYPE_NORMAL) {
		char *tmp = bloomfilter_to_base64(bf);
		
		if (tmp != NULL) {
			retval = tmp;
			free(tmp);
		}
	} else  {
		char *tmp = counting_bloomfilter_to_base64(cbf);
		
		if (tmp != NULL) {
			retval = tmp;
			free(tmp);
		}
	} 
	
	return retval;
}

bool Bloomfilter::fromBase64(const string &b64)
{
	// FIXME: how to determine if the b64 string contains a counting or 
	// non-counting bf? The base64_to_* functions don't.
	if (type == TYPE_NORMAL) {
		struct bloomfilter *tmp;
		tmp = base64_to_bloomfilter(b64.c_str(), b64.length());
		if (tmp == NULL) {
			HAGGLE_ERR("Bloomfilter assignment failed!\n");
			return false;
		} else {
			bloomfilter_free(bf);
			bf = tmp;
		}
	} else {
		struct counting_bloomfilter *tmp;
		tmp = base64_to_counting_bloomfilter(b64.c_str(), b64.length());
		if (tmp == NULL) {
			HAGGLE_ERR("Bloomfilter assignment failed!\n");
			return false;
		} else {
			counting_bloomfilter_free(cbf);
			cbf = tmp;
		}
	} 
	updates++;
	return true;
}

string Bloomfilter::toBase64NonCounting(void) const
{
	string retval;
	
	if (type == TYPE_NORMAL) {
		return toBase64();
	} else {
		char *tmp = counting_bloomfilter_to_noncounting_base64(cbf);
		if (tmp != NULL) {
			retval = tmp;
			free(tmp);
		}
	} 
	return retval;
}

Metadata *Bloomfilter::toMetadata(bool keep_counting) const
{
	char buf[40];
	string b64;
	Metadata *m = new XMLMetadata(BLOOMFILTER_METADATA);
	
	if (!m)
		return NULL;
	
	if (keep_counting) {
		b64 = toBase64();
		m->setParameter(BLOOMFILTER_METADATA_TYPE_PARAM, getTypeStr());
	} else {
		b64 = toBase64NonCounting();
		m->setParameter(BLOOMFILTER_METADATA_TYPE_PARAM, typeToStr(TYPE_NORMAL));
	}
	
	if (b64.length() == 0) {
		delete m;
		return NULL;
	}
	
	m->setContent(b64);

	snprintf(buf, 40, "%lf", error_rate);
	m->setParameter(BLOOMFILTER_METADATA_ERROR_RATE_PARAM, buf);
	snprintf(buf, 40, "%u", capacity);
	m->setParameter(BLOOMFILTER_METADATA_CAPACITY_PARAM, buf);
	snprintf(buf, 40, "%lu", numObjects());
	m->setParameter(BLOOMFILTER_METADATA_NUM_OBJECTS_PARAM, buf);
	
	return m;
}

Bloomfilter *Bloomfilter::fromMetadata(const Metadata& m)
{
	const char *param;
	double error_rate = default_error_rate;
	unsigned int capacity = default_capacity;
	Type_t type;
	
	if (!m.isName(BLOOMFILTER_METADATA))
		return NULL;
	
	param = m.getParameter(BLOOMFILTER_METADATA_TYPE_PARAM);
	
	if (!param)
		return NULL;
	
	type = strToType(param);
	
	if (type == TYPE_UNDEFINED)
		return NULL;
	
	param = m.getParameter(BLOOMFILTER_METADATA_ERROR_RATE_PARAM);
	
	if (param) {
		char *endptr = NULL;
#if defined(OS_WINDOWS)
		error_rate = atof(param);
		endptr = (char *)(param + strlen(param));
#else		
		error_rate = strtod(param, &endptr);
#endif
		if (endptr != '\0') {
			error_rate = default_error_rate;
		}
	}
	
	param = m.getParameter(BLOOMFILTER_METADATA_CAPACITY_PARAM);
	
	if (param) {
		char *endptr = NULL;
		capacity = (unsigned int)strtoul(param, &endptr, 10);
		
		if (endptr != '\0') {
			capacity = default_capacity;
		}
	}
	
	// We do not need to parse the NUM_OBJECTS_PARAM since that information 
	// is stored in the bloomfilter struct anyway. The PARAM is only for 
	// human readability
	
	return create_from_base64(type, m.getContent().c_str(), error_rate, capacity);
}

unsigned long Bloomfilter::numObjects(void) const
{
	if (type == TYPE_NORMAL) {
		return bloomfilter_get_n(bf);
	} else {
		return counting_bloomfilter_get_n(cbf);
	}
}

const unsigned char *Bloomfilter::getRaw(void) const
{
	return raw;
}

size_t Bloomfilter::getRawLen(void) const
{
	if (type == TYPE_NORMAL) {
		return (unsigned long)BLOOMFILTER_TOT_LEN(bf);
	} else {
		return (unsigned long)COUNTING_BLOOMFILTER_TOT_LEN(cbf);
	}
}

bool Bloomfilter::setRaw(const unsigned char *_bf, size_t _bf_len)
{
	if (!_bf || _bf_len == 0)
		return false;

	if (type == TYPE_NORMAL) {
		if (BLOOMFILTER_TOT_LEN(bf) != _bf_len) {
			HAGGLE_DBG("Old and new bloomfilter differ in length: %lu vs. %lu!\n",
				BLOOMFILTER_TOT_LEN(bf), (unsigned long)_bf_len);

			bloomfilter_free(bf);
			bf = (struct bloomfilter *)malloc(_bf_len);
			
			if (!bf) {
				HAGGLE_ERR("Could not allocate memory for new bloomfilter\n");
				return false;
			}
		} 
		memcpy(bf, _bf, _bf_len);
	} else {
		if (COUNTING_BLOOMFILTER_TOT_LEN(cbf) != _bf_len) {
			HAGGLE_DBG("Old and new bloomfilter differ in length: %lu vs. %lu!!\n",
				BLOOMFILTER_TOT_LEN(bf), (unsigned long)_bf_len);

			counting_bloomfilter_free(cbf);
			cbf = (struct counting_bloomfilter *)malloc(_bf_len);

			if (!cbf) {
				HAGGLE_ERR("Could not allocate memory for new bloomfilter\n");
				return false;
			}
		} 
		memcpy(cbf, _bf, _bf_len);
	}
	updates++;
	return true;
}

void Bloomfilter::reset()
{
	if (!raw)
		return;
	
	if (type == TYPE_NORMAL) {
		bloomfilter_free(bf);
		bf = bloomfilter_new(error_rate, capacity);
	} else {
		counting_bloomfilter_free(cbf);
		cbf = counting_bloomfilter_new(error_rate, capacity);
	}
	updates++;
}
