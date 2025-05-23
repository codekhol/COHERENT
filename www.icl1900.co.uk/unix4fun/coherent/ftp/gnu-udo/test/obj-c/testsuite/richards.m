/*                               -*- Mode: C -*- 
 * 
 * richards.m -- RichardsBenchmarks for Objective-C
 * 
 * Author          : Kresten Krab Thorup (krab@iesd.auc.dk)
 * Created On      : Fri Mar 26 17:44:14 1993
 * 
 */

#include <stdarg.h>
#include <objc/objc.h>
#include <objc/Object.h>
#include <stdio.h>

typedef id(*BLOCK)(id,id,id);

#include "richards.h"

#include <sys/time.h>
#include <sys/resource.h>


@interface Time : Object
+ initialize;

+(long) millisecondClockValue;
+(long) secondClockValue;
+(long) mSecClock;
@end

@implementation Time : Object

static long lastTime = 0;

+ initialize { [self mSecClock]; return self; }

+(long) millisecondClockValue { return [self mSecClock]; }
+(long) secondClockValue { return ([self mSecClock]/1000); }

+(long) mSecClock
{
 struct rusage          rusage;
 long                   theTime;
 long                   value;
   
        getrusage(RUSAGE_SELF, &rusage);
        theTime = (rusage.ru_utime.tv_sec * 1000) + (rusage.ru_utime.tv_usec /
 1000);
        value   = theTime - lastTime;
        lastTime        = theTime;
        return (value);
}

@end


int DeviceA;
int DeviceB;
int devicePacketKind;
int HandlerA;
int HandlerB;
int Idler;
id NoTask;
id NoWork;
int Worker;
int WorkPacketKind;

@implementation RBObject : Object 
  
- append: packet head: queueHead
{
  id mouse, link;
  [packet link: NoWork];
  if(NoWork==queueHead) return packet;
  mouse = queueHead;
  while(NoWork != (link = [mouse link]))
    mouse = link;
  [mouse link: packet];
  return queueHead;
}

+ initialize
{
  DeviceA = 5;
  DeviceB = 6;
  devicePacketKind = 1;
  HandlerA = 3;
  HandlerB = 4;
  Idler = 1;
  NoWork = nil;
  NoTask = nil;
  Worker = 2;
  WorkPacketKind = 2;
}  

+new
{
  id ss = [self alloc];
  if(ss == 0) 
    [self error: "Alloc failed"];
  else {
    [ss init];
    return ss;
  }
}

@end


@implementation HandlerTaskDataRecord : RBObject
{
  id workIn;
  id deviceIn;
}

- init
{
  workIn = deviceIn = NoWork;
}

- deviceIn
{
  return deviceIn;
}

- deviceIn: aPacket
{
  deviceIn = aPacket;
}

- deviceInAdd: packet
{
  deviceIn = [self append: packet head: deviceIn];
}

- workIn
{
  return workIn;
}

- workIn: aWorkQueue
{
  workIn = aWorkQueue;
}

- workInAdd: packet
{
  workIn = [self append: packet head: workIn];
}

@end


@implementation RichardsBenchmarks : RBObject
{
  id taskList;
  id currentTask;
  int currentTaskIdentity;
  id* taskTable;
  BOOL tracing;
  int layout;
  int queuePacketCount;
  int holdCount;
}

id doCreateDevice(id self, id work, id word) {
  id data;
  id functionWork;
  
  data = word;
  functionWork = work;
  if(NoWork == functionWork)
    if(NoWork == (functionWork = [data pending]))
      return [self wait];
    else {
      [data pending: NoWork];
      return [self queuePacket: functionWork];
    }
  else {
    [data pending: functionWork];
    return [self holdSelf];
  }
}

- createDevice: (int)identity priority: (int)priority work: work state: state
{
  return
    [self createTask: identity
	  priority: priority
	  work: work
	  state: state
	  function: doCreateDevice
	  data: [DeviceTaskDataRecord new]];
}


id doCreateHandler(id self, id work, id word) {
  id data;
  id workPacket;
  int count;
  id devicePacket;

  data = word;
  if(NoWork != work) 
    if(WorkPacketKind == [work kind])
      [data workInAdd: work];
    else
      [data deviceInAdd: work];
  if(NoWork == (workPacket = [data workIn]))
    return [self wait];
  else {
    count = [workPacket datum];
    if(count > 4) {
      [data workIn: [workPacket link]];
      return [self queuePacket: workPacket];
    } else {
      if(NoWork == (devicePacket = [data deviceIn]))
	return [self wait];
      else {
	[data deviceIn: [devicePacket link]];
	[devicePacket datum: ([workPacket data])[count]];
	[workPacket datum: count+1];
	return [self queuePacket: devicePacket];
      }
    }
  }
}

- createHandler: (int)identity priority: (int)priority work: work state: state 
{
  return 
    [self createTask: identity
	  priority: priority
	  work: work
	  state: state
	  function: doCreateHandler
	  data: [HandlerTaskDataRecord new]];
}



id doCreateIdler(id self, id work, id word) {
  id data;
  data = word;
  [data count: [data count] - 1];
  if(0 == [data count]) 
    return [self holdSelf];
  else
    if(0 == ([data control] & 1)) {
      [data control: [data control] / 2];
      return [self release: DeviceA];
    } else {
      [data control: ([data control] / 2) ^ 53256];
      return [self release: DeviceB];
    }
}

-createIdler: (int)identity priority: (int)priority work: work state: state 
{
  return [self createTask: identity
  	       priority: priority
	       work: work
	       state: state
	       function: doCreateIdler
	       data: [IdleTaskDataRecord new]];
}


-createPacket: link identity: (int)identity kind: (int)kind 
{
  return [Packet new: link
		 identity: identity
		 kind: kind];
}

-createTask: (int)identity priority: (int)priority work: work state: state function: (BLOCK)aBlock data: data 
{
  id t;
  t = [TaskControlBlock link: taskList
                        create: identity
			priority: priority
			initialWorkQueue: work
			initialState: state
			function: aBlock
			privateData: data
                        runIn: self];
  taskList = t;
  taskTable[identity] = t;
  return t;
}



id doCreateWorker(id self, id work, id word) {
  int i;
  id data;
  data = word;
  if(NoWork==work)
    return [self wait];
  else {
    [data destination: (HandlerA == [data destination] 
			? HandlerB : HandlerA)];
    [work identity: [data destination]];
    [work datum: 1];
    for(i=1; i<=4; i++) {
      [data count: [data count]+1];
      if([data count]>26) [data count: 1];
      ([work data])[i] = ((int)'A') + [data count] - 1;
    }
    return [self queuePacket: work];
  }
}

-createWorker: (int)identity priority: (int)priority work: work state: state 
{
  return
  [self createTask: identity
	priority: priority
	work: work
	state: state
	function: doCreateWorker
	data: [WorkerTaskDataRecord new]];
}

-findTask: (int)identity 
{
  id t = taskTable[identity];
  if(NoTask == t) [self error: "findTask failed"];
  return t;
}

-holdSelf
{
  holdCount += 1;
  [currentTask taskHolding: YES];
  return [currentTask link];
}

-initScheduler
{
  queuePacketCount = holdCount = 0;
  taskTable = (id*)malloc(7*sizeof(id*));
  {
    int i; for(i=1; i<7; i++) taskTable[i] = NoTask;
    taskTable[0] = (id)6;
  }
    
  taskList = NoTask;
  return self;
}


-queuePacket: packet 
{
  id t = [self findTask: [packet identity]];
  if(NoTask==t) return NoTask;
  queuePacketCount += 1;
  [packet link: NoWork];
  [packet identity: currentTaskIdentity];
  return [t addInput: packet checkPriority: currentTask];
}

-release: (int)identity 
{
  id t = [self findTask: identity];
  if(NoTask == t) return NoTask;
  [t taskHolding: NO];
  if([t priority] > [currentTask priority])
    return t;
  else
    return currentTask;
}

-wait 
{
  [currentTask taskWaiting: YES];
  return currentTask;
}



-schedule
{
  currentTask = taskList;
  while(NoTask != currentTask) {
    if([currentTask isTaskHoldingOrWaiting])
      currentTask = [currentTask link];
    else {
      currentTaskIdentity = [currentTask identity];
      currentTask = [currentTask runTask];
    }
  }
  return self;
}

-start
{
  id workQ;
  int mark1;
  int mark2;
  int mark3;
  int mark4;

  [self initScheduler];
  mark1 = [Time millisecondClockValue];
  printf( "Benchmark starting\n");

  [self	createIdler: Idler priority: 0 work: NoWork state: [TaskState running]];
  workQ = [self createPacket: NoWork
		identity: Worker
		kind: WorkPacketKind];
  workQ = [self createPacket: workQ
		identity: Worker
		kind: WorkPacketKind];
  [self createWorker: Worker 
        priority: 1000 work: workQ
        state: [TaskState waitingWithPacket]];

  workQ = [self createPacket: NoWork
		identity: DeviceA
		kind: devicePacketKind];
  workQ = [self createPacket: workQ
		identity: DeviceA
		kind: devicePacketKind];
  workQ = [self createPacket: workQ
		identity: DeviceA
		kind: devicePacketKind];

  [self createHandler: HandlerA
	priority: 2000
	work: workQ
	state: [TaskState waitingWithPacket]];

  workQ = [self createPacket: NoWork
		identity: DeviceB
		kind: devicePacketKind];
  workQ = [self createPacket: workQ
		identity: DeviceB
		kind: devicePacketKind];
  workQ = [self createPacket: workQ
		identity: DeviceB
		kind: devicePacketKind];


  [self createHandler: HandlerB
	priority: 3000
	work: workQ
	state: [TaskState waitingWithPacket]];

  [self createDevice: DeviceA
	priority: 4000
	work: NoWork
	state: [TaskState waiting]];

  [self createDevice: DeviceB
	priority: 5000
	work: NoWork
	state: [TaskState waiting]];

  printf( "Starting\n");
  mark2 = [Time millisecondClockValue];
  [self schedule];
  mark3 = [Time millisecondClockValue];

  printf( "Finished\n");
  printf( "QueuePacket count = %d\n", queuePacketCount);
  printf( "HoldCount = %d\n", holdCount);
  printf( "These results are: ");
  if(queuePacketCount==23246 && holdCount==9297)
    printf( "correct\n");
  else
    exit (1);

  mark4 = [Time millisecondClockValue];
  
  printf("*****Scheduler time = %dms; Total time = %dms\n",
          (mark3), 0);
  return self;
}  

+start
{
  return [[super alloc] start];
}

@end



@implementation WorkerTaskDataRecord : RBObject
{
  int destination;
  int count;
}

-init
{
  destination = HandlerA;
  count = 0;
  return self;
}

- (int)count { return count; }
- count: (int)aCount { count = aCount; }

- (int)destination { return destination; }
- destination: (int)aHandler { destination = aHandler; }

@end

@implementation TaskState : RBObject
{
  BOOL packetPending;
  BOOL taskWaiting;
  BOOL taskHolding;
}

-packetPending
{
  packetPending = YES;
  taskWaiting = NO;
  taskHolding = NO;
  return self;
}

-running
{
  packetPending = taskWaiting = taskHolding = NO;
  return self;
}

-waiting
{
  packetPending = taskHolding = NO;
  taskWaiting = YES;
  return self;
}

-waitingWithPacket
{
  taskHolding = NO;
  taskWaiting = packetPending = YES;
  return self;
}


-(BOOL)isPacketPending {
  return packetPending;
}

-(BOOL)isTaskHolding {
  return taskHolding;
}

-(BOOL)isTaskWaiting {
  return taskWaiting;
}

-taskHolding: (BOOL)aBoolean
{
  taskHolding = aBoolean;
}

-taskWaiting: (BOOL)aBoolean 
{
  taskWaiting = aBoolean;
}

-(BOOL)isRunning
{
  return((!packetPending) && (!taskWaiting) && (!taskHolding));
}

-(BOOL)isTaskHoldingOrWaiting
{
  return (taskHolding || ((!packetPending) && taskWaiting));
}

-(BOOL)isWaiting
{
  return ((!packetPending) && taskWaiting && (!taskHolding));
}

-(BOOL)isWaitingWithPacket
{
  return(packetPending && taskWaiting && (!taskHolding));
}

+packetPending
{
  return [[self alloc] packetPending];
}

+running
{
  return [[self alloc] running];
}

+waiting
{
  return [[self alloc] waiting];
}

+waitingWithPacket
{
  return [[self alloc] waitingWithPacket];
}

@end


@implementation IdleTaskDataRecord : RBObject
{
  int control;
  int count;
}

-init
{
  control = 1;
  count = 10000;
  return self;
}

-(int)control 
{
  return control;
}

-control: (int)aNumber
{
  control = aNumber;
}

-(int)count
{
  return count;
}

-count: (int)aCount
{
  count = aCount;
}

@end 


@implementation DeviceTaskDataRecord : RBObject
{
  id pending;
}

-init
{
  pending = NoWork;
  return self;
}

-pending
{
  return pending;
}

-pending: packet
{
  pending = packet;
}

@end



@implementation TaskControlBlock : TaskState
{
  id link;
  int identity;
  int priority;
  id input;
  id state;
  BLOCK function;
  id handle;
  id object;
}

-link: aLink identity: (int)anIdentity priority: (int)aPriority initialWorkQueue: anInitialWorkQueue initialState: anInitialState function: (BLOCK)aBlock privateData: aPrivateData runIn: anObject
{
  link = aLink;
  identity = anIdentity;
  priority = aPriority;
  input = anInitialWorkQueue;
  packetPending = [anInitialState isPacketPending];
  taskWaiting = [anInitialState isTaskWaiting];
  taskHolding = [anInitialState isTaskHolding];
  function = aBlock;
  handle = aPrivateData;
  object = anObject;
  return self;
}

-(int)identity
{
  return identity;
}

-link
{
  return link;
}

-(int)priority
{
  return priority;
}

-addInput: packet checkPriority: oldTask
{
  if(input==NoWork) {
    input = packet;
    packetPending = YES;
    if(priority > [oldTask priority]) return self;
  } else
    input = [self append: packet head: input];
  return oldTask;
}


-runTask
{
  id message;
  if([self isWaitingWithPacket]) {
    message = input;
    input = [message link];
    if(input==NoWork) [self running];
    else [self packetPending];
  } else
    message = NoWork;

  return (*function)(object, message, handle);
}

+link: aLink create: (int)anIdentity priority: (int)aPriority initialWorkQueue: initialWorkQueue initialState: initialState function: (BLOCK)aBlock privateData: privateData runIn: anObject
{
  return [[self new]
		link: aLink
		identity: anIdentity
		priority: aPriority
		initialWorkQueue: initialWorkQueue
		initialState: initialState
		function: aBlock
		privateData: privateData
                runIn: anObject];
}

@end



@implementation Packet : RBObject
{
  id link;
  int identity;
  int kind;
  int datum;
  int* data;
}

-link: aLink identity: (int)anIdentity kind: (int)aKind 
{
  link = aLink;
  identity = anIdentity; 
  kind = aKind;
  datum = 1;
  data = (int*)malloc(5*sizeof(int*));
  {
    int i; for(i=1; i<5; i++) data[i] = 0;
    data[0] = 4;
  }
  return self;
}

-(int*)data
{
  return data;
}

-(int)datum
{
  return datum;
}

-datum: (int)someData
{
  datum = someData;
}

-(int)identity
{
  return identity;
}

-identity: (int)anIdentity
{
  identity = anIdentity;
}

-(int)kind
{
  return kind;
}


-link
{
  return link;
}

-link: aWorkQueue 
{
  link = aWorkQueue;
}

+new: aLink identity: (int)anIdentity kind: (int)aKind 
{
  return [[self new]
		 link: aLink
		 identity: anIdentity
		 kind: aKind];
}

@end

main()
{
  [RichardsBenchmarks start];
  [RichardsBenchmarks start];

  __objc_print_dtable_stats();
  exit(0);
}
