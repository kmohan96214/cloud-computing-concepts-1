/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        
        // Create JOINREQ message
        msg.msgType = JOINREQ;
        msg.memberList = memberNode->memberList;
        msg.addr = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)&msg, sizeof(msg));

    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    free(memberNode);
    return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	MessageHdr* msg = (MessageHdr*) data;
    switch (msg->msgType)
    {
    case JOINREQ: {
        updateMemberList(msg);
        Address* to = msg->addr;
        sendMessage(to, JOINREP); 
    }
    break;
    case JOINREP: {
        updateMemberList(msg); 
        memberNode->inGroup = true;
    }
    break;
    case PING: {
        handlePing(msg);
    }
    break;
    default:
        break;
    }
    free(msg);
    return true;
}
/**
 * FUNCTION NAME: updateMemberList  
 * 
 * DESCRIPTION: If a node does not exist in the memberList, it will be pushed to the memberList.
 */
void MP1Node::updateMemberList(MessageHdr* msg) {
    // id, port, heartbeat, timestamp
    int id = 0;
	short port;
	memcpy(&id, &msg->addr->addr[0], sizeof(int));
	memcpy(&port, &msg->addr->addr[4], sizeof(short));
    long heartbeat = 1;
    long timestamp =  this->par->getcurrtime();
    if(getMemberIfPresent(id, port) != nullptr)
        return;
    MemberListEntry e(id, port, heartbeat, timestamp);
    memberNode->memberList.push_back(e);
    Address* added = createAddress(id,port);
    log->logNodeAdd(&memberNode->addr ,added);
    free(added);
}

void MP1Node::updateMemberList(MemberListEntry* e) {
    Address* addr = createAddress(e->id, e->port);
    
    if (*addr == memberNode->addr) {
        free(addr);
        return;
    }

    if (par->getcurrtime() - e->timestamp < TREMOVE) {
        log->logNodeAdd(&memberNode->addr, addr);
        MemberListEntry new_entry = *e;
        memberNode->memberList.push_back(new_entry);
    }
    delete addr;
}
/**
 * FUNCTION NAME: createAddress
 * 
 * DESCRIPTION: create new address
 */
Address* MP1Node::createAddress(int id, short port) {
    string ad = to_string(id) + ":" + to_string(port);
    Address* address = new Address(ad);
    return address;
}

/**
 * FUNCTION NAME: getMemberIfPresent 
 * 
 * DESCRIPTION: Return if a node is present in the member list 
 */
MemberListEntry* MP1Node::getMemberIfPresent(int id, short port) {
    int j =0;
    for (auto i = memberNode->memberList.begin(); i < memberNode->memberList.end(); i++, j++){
        if(i->getid() == id && i->getport() == port)
            return &memberNode->memberList[j];
    } 
    return nullptr;
}


/**
 * FUNCTION NAME: sendMessage 
 * 
 * DESCRIPTION: send message 
 */
void MP1Node::sendMessage(Address* to, MsgTypes t) {
    MessageHdr* msg = new MessageHdr();
    msg->msgType = t;
    msg->memberList = memberNode->memberList;
    msg->addr = &memberNode->addr;
    emulNet->ENsend(&memberNode->addr, to, (char*)msg, sizeof(*msg));    
}

/**
 * FUNCTION NAME: handlePing 
 * 
 * DESCRIPTION: The function handles the ping messages. 
 */
void MP1Node::handlePing(MessageHdr* msg) {
    size_t pos = msg->addr->getAddress().find(":");
	int id = stoi(msg->addr->getAddress().substr(0, pos));
	short port = (short)stoi(msg->addr->getAddress().substr(pos + 1, msg->addr->getAddress().size()-pos-1));
    
    MemberListEntry* pingFrom = getMemberIfPresent(id, port);
    if(pingFrom != nullptr){
        pingFrom->heartbeat++;
        pingFrom->settimestamp(par->getcurrtime());
    }else{
        updateMemberList(msg);
    }

    for(auto i : msg->memberList){
        MemberListEntry* node = getMemberIfPresent(i.getid(), i.getport());  
        // If a member is already present update if it has latest heartbeat
        if(node != nullptr){
            if(i.getheartbeat() > node->heartbeat){
                node->heartbeat = i.getheartbeat();
                node->timestamp = par->getcurrtime();
            }
        } else {
            // update the list
            updateMemberList(&i);
        }
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

    memberNode->heartbeat++;

    // delete members
    for (auto i = memberNode->memberList.begin(); i < memberNode->memberList.end(); i++) {
        if(par->getcurrtime() - i->gettimestamp() >= TREMOVE) {
            Address* toRemove = createAddress(i->getid(), i->getport());
            log->logNodeRemove(&memberNode->addr, toRemove);
            memberNode->memberList.erase(i--);
            delete toRemove;
        }
    }

    // send PING to the members of memberList
    for (auto node : memberNode->memberList) {
        Address* address = createAddress(node.getid(), node.getport());
        sendMessage(address, PING);
        free(address);
    }
    return;
}


/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}