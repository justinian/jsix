#pragma once
/// \file process.h
/// Definition of process kobject types

#include "objects/kobject.h"
#include "page_table.h"

class process :
	public kobject
{
public:
	/// Constructor.
	/// \args pml4  Root of the process' page tables
	process(page_table *pml4);

	/// Destructor.
	virtual ~process();

	/// Terminate this process.
	/// \arg code   The return code to exit with.
	void exit(unsigned code);

	/// Update internal bookkeeping about threads.
	void update();

	/// Get the process' page table root
	page_table * pml4() { return m_pml4; }

	/// Create a new thread in this process
	/// \args priority  The new thread's scheduling priority
	/// \returns        The newly created thread object
	thread * create_thread(uint8_t priorty);

	/// Inform the process of an exited thread
	/// \args th  The thread which has exited
	/// \returns  True if this thread ending has ended the process
	bool thread_exited(thread *th);

private:
	uint32_t m_return_code;

	page_table *m_pml4;
	kutil::vector<thread*> m_threads;

	static kutil::vector<process*> s_processes;
};
