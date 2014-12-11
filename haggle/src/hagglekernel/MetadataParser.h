#ifndef _METADATAPARSER_H
#define _METADATAPARSER_H

#if defined(ENABLE_METADATAPARSER)

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Platform.h>
#include "Metadata.h"

using namespace haggle;

class XMLMetadata;
/**
   MetadataParser can be inherited by classes that want to parse
   specific metadata as it is created. Metadata that matches the
   registered key will be passed to the MetadataParser in the
   onParseMetadata() function. Once parsed, the metadata can be
   accepted, or rejected, by returning true or false, respectively.

   In Haggle, Managers are intended to be MetadataParsers in order to
   check the consistency of manager specific metadata before the
   metadata is excepted as valid. This has the benefit that data
   objects are not created from invalid metadata, and processed
   unnessecarily before being discovered as bad. Hence, there is a
   possibility to do an "early reject" on incoming data objects.
   
   NOTE: Currently this feature is disabled, due to problems with
   multiple inheritence on Windows (mobile).
 */
class MetadataParser {
        friend class XMLMetadata;
	static unsigned int num;
	const string parsekey;
	typedef Map<const string, MetadataParser *> registry_t;
	static registry_t registry;
	static MetadataParser *getParser(string& key);
protected:
	MetadataParser(const string _parsekey);
	virtual ~MetadataParser() = 0;
        /**
           Derived classes should implement this function in order to
           receive the metadata matching the registered key, as
           indicated in the constructor.

           The metadata 'md' passed is only the metadata beginning at
           the matching key, including all children metadata.

           The parsing function should return 'false' in order to
           reject the metadata, or 'true' do accept it. If the derived
           class does not implement this function, the metadata will
           be accepted by default.
         */
	virtual bool onParseMetadata(Metadata *md) { return false; }
};

#endif /* ENABLE_METADATAPARSER */

#endif  /* _METADATAPARSER_H */
