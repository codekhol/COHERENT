@interface RBObject : Object 
- append: packet head: queueHead;
+ initialize;
@end
@interface HandlerTaskDataRecord : RBObject
{
  id workIn;
  id deviceIn;
}
- init;
- deviceIn;
- deviceIn: aPacket;
- deviceInAdd: packet;
- workIn;
- workIn: aWorkQueue;
- workInAdd: packet;
@end
@interface RichardsBenchmarks : RBObject
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
- createDevice: (int)identity priority: (int)priority work: work state: state;
- createHandler: (int)identity priority: (int)priority work: work state: state ;
-createIdler: (int)identity priority: (int)priority work: work state: state ;
-createPacket: link identity: (int)identity kind: (int)kind ;
-createTask: (int)identity priority: (int)priority work: work state: state function: (BLOCK)aBlock data: data ;
-createWorker: (int)identity priority: (int)priority work: work state: state ;
-findTask: (int)identity ;
-holdSelf;
-initScheduler;
-queuePacket: packet ;
-release: (int)identity ;
-wait ;
-schedule;
-start;
+start;
@end
@interface WorkerTaskDataRecord : RBObject
{
  int destination;
  int count;
}
-init;
- (int)count;
- count: (int)aCount;
- (int)destination;
- destination: (int)aHandler;
@end
@interface TaskState : RBObject
{
  BOOL packetPending;
  BOOL taskWaiting;
  BOOL taskHolding;
}
-packetPending;
-running;
-waiting;
-waitingWithPacket;
-(BOOL)isPacketPending;
-(BOOL)isTaskHolding;
-(BOOL)isTaskWaiting;
-taskHolding: (BOOL)aBoolean;
-taskWaiting: (BOOL)aBoolean ;
-(BOOL)isRunning;
-(BOOL)isTaskHoldingOrWaiting;
-(BOOL)isWaiting;
-(BOOL)isWaitingWithPacket;
+packetPending;
+running;
+waiting;
+waitingWithPacket;
@end
@interface IdleTaskDataRecord : RBObject
{
  int control;
  int count;
}
-init;
-(int)control ;
-control: (int)aNumber;
-(int)count;
-count: (int)aCount;
@end 
@interface DeviceTaskDataRecord : RBObject
{
  id pending;
}
-init;
-pending;
-pending: packet;
@end
@interface TaskControlBlock : TaskState
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
-link: aLink identity: (int)anIdentity priority: (int)aPriority initialWorkQueue: anInitialWorkQueue initialState: anInitialState function: (BLOCK)aBlock privateData: aPrivateData runIn: anObject;
-(int)identity;
-link;
-(int)priority;
-addInput: packet checkPriority: oldTask;
-runTask;
+link: link create: (int)identity priority: (int)priority initialWorkQueue: initialWorkQueue initialState: initialState function: (BLOCK)aBlock privateData: privateData runIn: anObject;
@end
@interface Packet : RBObject
{
  id link;
  int identity;
  int kind;
  int datum;
  int* data;
}
-link: aLink identity: (int)anIdentity kind: (int)aKind ;
-(int*)data;
-(int)datum;
-datum: (int)someData;
-(int)identity;
-identity: (int)anIdentity;
-(int)kind;
-link;
-link: aWorkQueue ;
+new: link identity: (int)identity kind: (int)kind ;
@end
