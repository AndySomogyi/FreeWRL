/*	Copyright: 	� Copyright 2003 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	CABufferList.cpp
	
=============================================================================*/

#include "CABufferList.h"

void		CABufferList::AllocateBuffers(UInt32 nBytes)
{
	if (nBytes <= GetNumBytes()) return;
	
	if (mNumberBuffers > 1)
		// align successive buffers for Altivec and to take alternating
		// cache line hits by spacing them by odd multiples of 16
		nBytes = (nBytes + (0x10 - (nBytes & 0xF))) | 0x10;
	UInt32 memorySize = nBytes * mNumberBuffers;
	Byte *newMemory = new Byte[memorySize], *p = newMemory;
	if (int(newMemory) & 0x0F) {
		// implementation gave us a non-aligned buffer, try again with 15 bytes more
		delete[] newMemory;
		newMemory = new Byte[memorySize + 0xF];
		check(sizeof(Byte *) == sizeof(UInt32));
		p = (Byte *)(UInt32(newMemory + 0x10) & 0xF);
	}
	
	AudioBuffer *buf = mBuffers;
	for (UInt32 i = mNumberBuffers; i--; ++buf) {
		if (buf->mData != NULL && buf->mDataByteSize > 0)
			// preserve existing buffer contents
			memcpy(p, buf->mData, buf->mDataByteSize);
		buf->mDataByteSize = nBytes;
		buf->mData = p;
		p += nBytes;
	}
	Byte *oldMemory = mBufferMemory;
	mBufferMemory = newMemory;
	delete[] oldMemory;
}

void		CABufferList::DeallocateBuffers()
{
	if (mExternallyOwnedBufferMemory)
		return;
	
	AudioBuffer *buf = mBuffers;
	for (UInt32 i = mNumberBuffers; i--; ++buf) {
		buf->mData = NULL;
		buf->mDataByteSize = 0;
	}
	if (mBufferMemory != NULL) {
		delete[] mBufferMemory;
		mBufferMemory = NULL;
	}
}

void		CABufferList::UseExternalBuffer(Byte *ptr, UInt32 nBytes)
{
	DeallocateBuffers();
	
}

