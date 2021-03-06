// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages+numPagesAllocated <= NumPhysPages);		// check we're not trying
										// to run anything too big --
										// at least until we have
										// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = i+numPagesAllocated;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
    pageTable[i].shared= FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
 
    numPagesAllocated += numPages;

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        vpn = noffH.code.virtualAddr/PageSize;
        offset = noffH.code.virtualAddr%PageSize;
        entry = &pageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        vpn = noffH.initData.virtualAddr/PageSize;
        offset = noffH.initData.virtualAddr%PageSize;
        entry = &pageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(AddrSpace *parentSpace)
{
    numPages = parentSpace->GetNumPages();
   // printf("Intial numPagesAllocated %d\n",numPagesAllocated );
    unsigned  size = numPages * PageSize;
    int i;
    int temp=0;
    ASSERT(numPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                        numPages, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
       // printf("parent physical page number %d, machine pageTable  %d \n", parentPageTable[i].physicalPage, machine->pageTable[i].physicalPage);
        if(parentPageTable[i].shared==FALSE){

            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = temp+numPagesAllocated;
            pageTable[i].valid = parentPageTable[i].valid;
            pageTable[i].use = parentPageTable[i].use;
            pageTable[i].dirty = parentPageTable[i].dirty;
            pageTable[i].shared = parentPageTable[i].shared;
            pageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
            temp++; 
           // printf("first addrshared space, physical page %d, pageTable.readOnly %d, pageTable.shared %d,temp %d\n",pageTable[i].physicalPage, pageTable[i].readOnly,pageTable[i].shared,temp);
                                     			// pages to be read-only
        }
        else{
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = parentPageTable[i].physicalPage;
            pageTable[i].valid = parentPageTable[i].valid;
            pageTable[i].use = parentPageTable[i].use;
            pageTable[i].dirty = parentPageTable[i].dirty;
            pageTable[i].shared = parentPageTable[i].shared;         
            pageTable[i].readOnly = parentPageTable[i].readOnly; 
        // printf("second addrshared space, pageTable.physicalPage %d, pageTable.readOnly %d, pageTable.shared %d,temp %d\n",pageTable[i].physicalPage, pageTable[i].readOnly,pageTable[i].shared,temp);
                  }
    }

    // Copy the contents

    unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    unsigned startAddrChild = numPagesAllocated*PageSize;
   // printf("startAddrChild %d, startAddrParent %d\n",  startAddrChild, startAddrParent);
    unsigned tempmem;
    tempmem=0;
    for (i=0; i<size; i++) {
        if(parentPageTable[i/PageSize].shared==FALSE){
            machine->mainMemory[startAddrChild+tempmem] = machine->mainMemory[startAddrParent+i];
            tempmem++;
        }
    }

    numPagesAllocated += temp;
   // printf("final numPagesAllocated %d\n", numPagesAllocated );
}

unsigned
AddrSpace::SharedSpace(unsigned int NumBytes)
{
    unsigned i=NumBytes;
    unsigned OldSize=numPages;
    unsigned PageToBeAllocated=NumBytes/PageSize;
    if(PageToBeAllocated*PageSize!=NumBytes)
        PageToBeAllocated++;
    //printf("page to PageToBeAllocated:, PageSize, NumBytes , numPages: %d  %d %d %d\n",PageToBeAllocated, PageSize, NumBytes,numPagesAllocated);
    //ASSERT(numPages+PageToBeAllocated <= NumPhysPages);   
    //TranslationEntry* Old = pageTable;
    TranslationEntry* newpageTable=new TranslationEntry[PageToBeAllocated+numPages];
    for(i=0; i<numPages;i++)
    {
            
        newpageTable[i].virtualPage = i;
        newpageTable[i].physicalPage = pageTable[i].physicalPage;
        newpageTable[i].valid = pageTable[i].valid;
        newpageTable[i].use = pageTable[i].use;
        newpageTable[i].dirty = pageTable[i].dirty;
        newpageTable[i].readOnly = pageTable[i].readOnly;
        newpageTable[i].shared= pageTable[i].shared;
    }
    //pageTable=newpageTable;
    delete pageTable;
    //TranslationEntry* 
    pageTable=new TranslationEntry[PageToBeAllocated+numPages];
    for(i=0; i<numPages;i++)
    {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = newpageTable[i].physicalPage;
        pageTable[i].valid = newpageTable[i].valid;
        pageTable[i].use = newpageTable[i].use;
        pageTable[i].dirty = newpageTable[i].dirty;
        pageTable[i].readOnly = newpageTable[i].readOnly;
        pageTable[i].shared= newpageTable[i].shared;
     //   printf("first shared space, pageTable.physicalPage %d, pageTable.readOnly %d, pageTable.shared %d\n",pageTable[i].physicalPage, pageTable[i].readOnly,pageTable[i].shared);
    }
    delete newpageTable;
    
    for(i=numPages;i<numPages+PageToBeAllocated;i++)
    {

            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = i-numPages+numPagesAllocated;
         pageTable[i].valid = TRUE;
         pageTable[i].use = FALSE;
         pageTable[i].dirty = FALSE;
         pageTable[i].readOnly = FALSE;
        pageTable[i].shared=TRUE;
//printf("second shared space, physical page %d, pageTable.readOnly %d, pageTable.shared %d\n",pageTable[i].physicalPage,pageTable[i].readOnly,pageTable[i].shared);
    }
  //  printf("HELLO from the SharedSpace\n");

    numPagesAllocated+=PageToBeAllocated;
    numPages+=PageToBeAllocated;
    machine->pageTable=pageTable;
    machine->pageTableSize = numPages;
    //printf("Returned Virtual address %d, numPagesAllocated %d\n", OldSize*PageSize,numPagesAllocated);
    return OldSize*PageSize;
}
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}

TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}
