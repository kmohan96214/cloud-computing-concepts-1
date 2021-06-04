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
    this->memberTable.clear();
    this->time = 0;
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
	memberNode->timeOutCounter = 1;
    initMemberListTable(memberNode);
    
    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
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
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        // msg->from = (Address*)malloc(sizeof(memberNode->addr));
        msg->from = memberNode->addr;
        printAddress(&msg->from);
        // memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        printAddress(joinaddr);
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
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
   // MY COMMENTS - make is active true false bfailed true
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
	/*
	 * Your code goes here
	 */
    MessageHdr *msg = (MessageHdr*)data;

    switch(msg->msgType) {
        case JOINREQ: {
            addToMemberList(msg);
            MessageHdr *message = (MessageHdr*)malloc(sizeof(char));
            message->msgType = JOINREP;
            log->logNodeAdd(&this->memberNode->addr, &msg->from);
            this->sendMessage(message, &msg->from);
            break;
        }
        case JOINREP: {
            break;
        }
        case PING: {
            //log->logNodeAdd(&this->memberNode->addr, &msg->from);
            this->updateMemberList(msg);
            break;
        }
        default: break;
    }

    return true;
}

bool MP1Node::addToMemberList(MessageHdr *msg) {
    size_t pos = msg->from.getAddress().find(":");
    MemberListEntry entry(stoi(msg->from.getAddress().substr(0, pos)), (short)stoi(msg->from.getAddress().substr(pos + 1, msg->from.getAddress().size()-pos-1)));
    this->memberNode->memberList.emplace_back(entry);
    this->memberTable[stoi(msg->from.getAddress().substr(0, pos))] = this->memberNode->timeOutCounter;
    //log->logNodeAdd(&this->memberNode->addr, msg->from);
    return true;
}

bool MP1Node::sendMessage(MessageHdr *msg, Address *to) {
    Address add(this->memberNode->addr);
    msg->from = add;
    this->emulNet->ENsend(&this->memberNode->addr, to, (char*)msg, 5);
    return true;
}

void MP1Node::updateMemberList(MessageHdr *msg) {
    size_t pos = msg->to.getAddress().find(":");
    if (this->memberTable[stoi(msg->to.getAddress().substr(0, pos))] == 0) {
        size_t pos = msg->to.getAddress().find(":");
        MemberListEntry entry(stoi(msg->to.getAddress().substr(0, pos)), (short)stoi(msg->to.getAddress().substr(pos + 1, msg->to.getAddress().size()-pos-1)));
        log->logNodeAdd(&this->memberNode->addr, &msg->to);
    }

    this->memberTable[stoi(msg->to.getAddress().substr(0, pos))] = this->memberNode->timeOutCounter;
    return;
}

// void ipToInt(char* addr) {
//     int ans = 0;
//     int factor = 1;
//     for (int i = 3; i >= 0; i--) {
//         ans += 
//     }
// }

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
    
    for (auto node : this->memberTable) {
        if (this->memberNode->timeOutCounter - node.second > TFAIL) {
            this->memberTable[node.first] = -1;
            
            for (auto i = this->memberNode->memberList.begin(); i < this->memberNode->memberList.end(); i++) {
                int n = i->id;
                if (this->memberNode->timeOutCounter - this->memberTable[n] > TFAIL) {
                    Address toRemove;

                    memset(&toRemove, 0, sizeof(Address));
                    *(int *)(&toRemove.addr) = n;
                    *(short *)(&toRemove.addr[4]) = 0;
                    // *(int *)(&toRemove->addr) = n;
                    // *(short *)(toRemove->addr[4]) = 0;
                    log->logNodeRemove(&this->memberNode->addr, &toRemove);
                    this->memberNode->memberList.erase(i);
                    i--;
                }
            }
        }
    }

    for (auto i: this->memberNode->memberList) {
        MessageHdr *msg = (MessageHdr*)malloc(sizeof(MessageHdr)*3);
        msg->msgType = PING;
        Address to;

        memset(&to, 0, sizeof(Address));
        *(int *)(&to.addr) = i.id;
        *(short *)(&to.addr[4]) = 0;
        Address from;

        memset(&from, 0, sizeof(Address));
        string address = this->memberNode->addr.getAddress();
        size_t pos = address.find(":");
		int id = stoi(address.substr(0, pos));
		short port = (short)stoi(address.substr(pos + 1, address.size()-pos-1));
        *(int *)(&from.addr) = id;
        *(short *)(&from.addr[4]) = port;
		// memcpy(from, &id, sizeof(int));
		// memcpy(&from[4], &port, sizeof(short));


        //*(int *)(&from.addr) = i.id;
        //*(short *)(&from.addr[4]) = 0;
        // Address *to = (Address*)malloc(sizeof(Address));
        // *(int *)(&to->addr) = i.id;
        //to->addr[0] = char(i.id + '0');
        msg->to = from;
        //Address from(this->memberNode->addr.getAddress());
        msg->from = to;
        sendMessage(msg, &to);
    }

    this->memberNode->timeOutCounter++;

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
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
