# celonis-assignment
[**presentation**](https://docs.google.com/presentation/d/1d-PVI2-qrn1iCgPpd78NN5Z-pNJhS7gDiPtk-ccnI50/edit#slide=id.g26bc880654_0_1183)

A (as of yet still under development) key-value store with Multithreading and multi-client support.

Network and internal communication is handled via [**zeromq**](http://zeromq.org/). It the application also uses [**boost**](http://www.boost.org/).

##### Current network message formats:
* **PUT** 		:   \[\'p\', strlen(key) {binary ull}, key, value\]
* **GET** 		:   \[\'g\', key\]
* **DELETE** 	: 	\[\'d\', key\]

Current flow is structured in 3 parts, part 1 and 2 are mostly done, part 3 is still under development:

##### I Network, KVManager
1. Workers parse the message and call the corresponding function in the KV manager
2. The KV manager hashes the string and looks for a corresponding entry. An entry consists of
   * the segment number (which identifies the file the date is to be saved to), 
   * the the id of the first page where the entries can be found.
   * 16-bit flag value with meta-information about the entry. 
3. After it finds the entry (or creates a new one, if needed) it asks the BufferManager to pin the page corresponding to the entry in-memory.

##### II BufferManager
1. The **Buffer manager(BM)** checks if the page is already loaded and tries to lock its corresponding segment (either shared or exclsivelt depending on the use case)
   * at this point the BM releases the lock guarding it's data structure before trying to aquire the segment lock
   * this is done to ensure that the BM is as responsive to multiple threads as possible
   * as a side effect, in the (presumably) rare case when the page was evicted there is a retry.
2. If the page is not already loaded it asks its eviction policy (first template param) for the next victim
   * the eviction policy also gets an exlusive lock on the segment it want to evict
3. It then removes the entry from its stored entries and marks the new entry as being in loading
4. after that it releases it's data structure lock and asks its ioPolicy (second template param) to flush the page and load the new one.
5. after loading is done it releases the segment's shared mutex and does some more internal bookkeping.
6. unpinning pages is simply a matter of realeasing the lock.

##### III KVManager page format (this part is still under development).
1. afterwords the KVManager has a page pointing to the first entry of hash(key).
   * The page layout consits of:
      * a `uint64_t` *pageId* which points to the next page in the same bucket (or to itself if it is the end)
      * a `uint16_t` *NumEntries* signaling the number of entries stored in this page
      * a `uint16_t` *startOffset* signaling the start offset on this page (needed for deletions and multi-page reconds)
      * a `uint16_t[NumEntries]` array showing ths size of each entry. If the most significant bit is set, the entry is a multi-page entry
      * the keys and values themselves are store subsequently in **_reversed_** order starting at the end of the page.
        * The orders is reversed to allow for better page filling
2. For **get/del** it then looks through the entries until it finds a key matching the key it is looking for (pinning and unpinning pages as needed)
   * in the case of **get**, it then proceeds to reading the value stored after the matching key and sends it as a reply.
   * in the case of **delte**, it switches to exclusive locks and proceeds to removing the key and its value. packing and collapsing pages as needed.
3. for **put** it looks for a page with enough free space (or the last page if none exist) and it adds the pair there.

##### Other things of note:
* More metadata in the page headers might allow for a faster delete(things like current size, maby having the list be a skip list)
* The application can be distributed fairly easily. the communcation between the server entrypoint and the workers just needs to be changed
from intra-process to network communication
   * the requrests should also be distributed to workers based on cache
* Similarily to the previous point, the requests can be split between different numa nodes based on their hash for increased performance
