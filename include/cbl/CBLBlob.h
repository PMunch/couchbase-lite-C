//
// CBLBlob.h
//
// Copyright (c) 2018 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once
#include "CBLBase.h"
#include "fleece/Fleece.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup blobs Blobs
    @{
    A CBLBlob is a binary data blob associated with a document.
 */

    extern const FLSlice kCBLTypeProperty;
    extern const FLSlice kCBLBlobType;

    extern const FLSlice kCBLBlobDigestProperty;
    extern const FLSlice kCBLBlobLengthProperty;
    extern const FLSlice kCBLBlobContentTypeProperty;


    /** Returns true if a dictionary in a document is a blob reference.
        If so, you can call \ref cbl_blob_get to access it. */
    bool cbl_isBlob(FLDict) CBLAPI;

    
    CBL_REFCOUNTED(CBLBlob*, blob);

    /** Returns a CBLBlob object corresponding to a blob dictionary in a document.
        @param blobDict  A dictionary in a document.
        @return  A CBLBlob instance for this blob, or NULL if the dictionary is not a blob. */
    const CBLBlob* cbl_blob_get(FLDict blobDict) CBLAPI;


#pragma mark - BLOB METADATA:

    /** Returns the length in bytes of a blob's content. */
    uint64_t cbl_blob_length(const CBLBlob* _cbl_nonnull) CBLAPI;

    /** Returns the cryptographic digest of a blob's content. */
    const char* cbl_blob_digest(const CBLBlob* _cbl_nonnull) CBLAPI;

    /** Returns a blob's MIME type, if its metadata has a kCBLBlobContentTypeProperty. */
    const char* cbl_blob_contentType(const CBLBlob* _cbl_nonnull) CBLAPI;

    /** Returns a blob's metadata. This includes the `digest`, `length` and `content_type`
        properties, as well as any custom ones that may have been added. */
    FLDict cbl_blob_properties(const CBLBlob* _cbl_nonnull) CBLAPI;


#pragma mark - READING:

    /** Reads the blob's contents into memory and returns them.
        You are responsible for calling \ref FLSliceResult_Free on the returned data when done.
        @warning  This can potentially allocate a very large heap block! */
    FLSliceResult cbl_blob_loadContent(const CBLBlob* _cbl_nonnull, CBLError *outError) CBLAPI;

    /** A stream for reading a blob's content. */
    typedef struct CBLBlobReadStream CBLBlobReadStream;

    /** Opens a stream for reading a blob's content. */
    CBLBlobReadStream* cbl_blob_openContentStream(const CBLBlob* _cbl_nonnull, CBLError *outError) CBLAPI;

    /** Reads data from a blob.
        @param stream  The stream to read from.
        @param dst  The address to copy the read data to.
        @param maxLength  The maximum number of bytes to read.
        @param outError  On failure, an error will be stored here if non-NULL.
        @return  The actual number of bytes read; 0 if at EOF, -1 on error. */
    ssize_t cbl_blobreader_read(CBLBlobReadStream* stream _cbl_nonnull,
                                void *dst _cbl_nonnull,
                                size_t maxLength,
                                CBLError *outError) CBLAPI;

    /** Closes a CBLBlobReadStream. */
    void cbl_blobreader_close(CBLBlobReadStream*) CBLAPI;


#pragma mark - CREATING:

    /** Creates a new blob given its contents as a single block of data.
        @note  You are responsible for releasing the CBLBlob, but not until after its document
                has been saved.
        @param contentType  The MIME type (optional).
        @param contents  The data's address and length.
        @return  A new CBLBlob instance. */
    CBLBlob* cbl_blob_createWithData(const char *contentType,
                                     FLSlice contents) CBLAPI;

    /** A stream for writing a new blob to the database. */
    typedef struct CBLBlobWriteStream CBLBlobWriteStream;

    /** Opens a stream for writing a new blob.
        You should call \ref cbl_blobwriter_write one or more times to write the data,
        then \ref cbl_blob_createWithStream to create the blob.

        If for some reason you need to abort, just call \ref cbl_blobwriter_close. */
    CBLBlobWriteStream* cbl_blobwriter_new(CBLDatabase *db _cbl_nonnull,
                                           CBLError *outError) CBLAPI;

    /** Closes a blob-writing stream, if you need to give up without creating a CBLBlob. */
    void cbl_blobwriter_close(CBLBlobWriteStream*) CBLAPI;

    /** Writes data to a blob.
        @param writer  The stream to write to.
        @param data  The address of the data to write.
        @param length  The length of the data to write.
        @param outError  On failure, error info will be written here.
        @return  True on success, false on failure. */
    bool cbl_blobwriter_write(CBLBlobWriteStream* writer _cbl_nonnull,
                              const void *data _cbl_nonnull,
                              size_t length,
                              CBLError *outError) CBLAPI;

    /** Creates a new blob after its data has been written to a \ref CBLBlobWriteStream.
        You should then add the blob to a mutable document as a property -- see
        \ref CBLMutableDict_SetBlob and \ref CBLMutableArray_SetBlob.
        @note  You are responsible for releasing the CBLBlob reference.
        @note  Do not free the stream; the blob will do that.
        @param contentType  The MIME type (optional).
        @param writer  The blob-writing stream the data was written to.
        @return  A new CBLBlob instance. */
    CBLBlob* cbl_blob_createWithStream(const char *contentType,
                                       CBLBlobWriteStream* writer _cbl_nonnull) CBLAPI;

#pragma mark - FLEECE UTILITIES:

    /** Returns true if a value in a document is a blob reference.
        If so, you can call \ref FLValue_GetBlob to access it. */
    static inline bool FLValue_IsBlob(FLValue v) {
        return cbl_isBlob(FLValue_AsDict(v));
    }

    /** Instantiates a CBLBlob object corresponding to a blob dictionary in a document.
        @param value  The value (dictionary) in the document.
        @return  A CBLBlob instance for this blob, or NULL if the value is not a blob.
        @note You are responsible for releasing the CBLBlob object.  */
    static inline const CBLBlob* FLValue_GetBlob(FLValue value) {
        return cbl_blob_get(FLValue_AsDict(value));
    }

    /** Stores a blob in a mutable array. */
    void CBLMutableArray_SetBlob(FLMutableArray array _cbl_nonnull,
                                 uint32_t index,
                                 CBLBlob* blob _cbl_nonnull);

    /** Stores a blob in a mutable dictionary. */
    void CBLMutableDict_SetBlob(FLMutableDict dict _cbl_nonnull,
                                FLString key,
                                CBLBlob* blob _cbl_nonnull);

/** @} */

#ifdef __cplusplus
}
#endif
