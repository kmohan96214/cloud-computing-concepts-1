# Cloud-Computing-Concepts-Part-1 : 
*	https://www.coursera.org/specializations/cloud-computing

# What is the project? 
*	The project is about implementing a **Gossip Protocol**.
*	The main functionalities of the project :
	* **Introduction** : 
	Each new peer contacts a well-known peer (the introducer) to join the group. 
	* **Membership** : 
	A membership protocol satisfies completeness all the time (for joins and failures), and accuracy when there are no message delays or losses (high accuracy when there are losses or delays). 
# Detail & Principle :
*	Data Structure of Message : 
```cpp
typedef struct MessageHdr {
	enum MsgTypes msgType; 
	vector< MemberListEntry> member_vector; // membership list of source
	Address* addr; // the source of this message
}MessageHdr;
```
*	Principle of **Gossip Protocol** :
[reference](https://github.com/kmohan96214/cloud-computing-concepts-1/blob/main/GossipStyleDetection.pdf)

![image](https://github.com/kmohan96214/cloud-computing-concepts-1/blob/main/gossip.png)

	

	
	
# How do I run the Grader on my computer ?
*	There is a grader script GraderNew.sh. It tests your implementation of membership protocol in 3 scenarios.
	* Single node failure
	* Multiple node failure
	* Single node failure under a lossy network.
```
	$ chmod +x Grader.sh
	$ ./Grader.sh
```
# Result
*	Points achieved: 90 out of 90 [100%]
	
