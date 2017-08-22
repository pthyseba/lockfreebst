#include <thread>
#include <atomic>
#include <iostream>
#include <vector>


struct Node
{
  int iValue;
  std::atomic<bool> iDeleted;
  std::atomic<Node*> iLeft, iRight;

  Node() : iValue(0), iDeleted(false), iLeft(nullptr), iRight(nullptr) {}
};

std::atomic<Node*> gAnchor;

bool Contains(std::atomic<Node*>& aRoot, int aSearch)
{
  Node* curNode = aRoot.load();
  if (curNode == nullptr)
  {
    return false;
  } 
  else if (curNode->iValue == aSearch)
  {
    return !curNode->iDeleted;
  }
  else if (curNode->iValue < aSearch)
  {
    return Contains(curNode->iRight, aSearch);
  }
  else
  {
    return Contains(curNode->iLeft, aSearch);
  }
}

void Add(std::atomic<Node*>& aRoot, int aValue)
{
  Node* newNode = new Node();
  newNode->iValue = aValue;
  
  Node* curNode = aRoot.load();
 
  if (curNode == nullptr)
  {
    // either root is still nullptr, then exchange with newnode
    // or other thread has already added node. Start over.
    bool updated = aRoot.compare_exchange_strong(curNode, newNode);
    if (updated)
    {
      return;
    }
  }

  // Either other thread modified root or root was not null; either way, tree is no longer empty.
  // curNode contains actual non-null value of aRoot      
  for(;;)
  {  
    // descend into tree
    int curValue = curNode->iValue;
    bool curDeleted = curNode->iDeleted.load();
    if (curValue == aValue)
    {
      if (curDeleted)
      {
        curNode->iDeleted.compare_exchange_strong(curDeleted, false);
      }
      // no duplicates
      delete newNode;
      return;
    }
    if (aValue < curValue)
    {
      // Try to insert new node as left child of current node
      Node* nullNode = nullptr;
      bool updated = curNode->iLeft.compare_exchange_strong(nullNode, newNode);
      if (updated)
      {
        return;
      }
      curNode = curNode->iLeft.load();
    }
    else if (aValue > curValue)
    {
      // Try to insert new node as right child of current node
      Node* nullNode = nullptr;
      bool updated = curNode->iRight.compare_exchange_strong(nullNode, newNode);
      if (updated)
      {
        return;
      }
      curNode = curNode->iRight.load();
    }  
  }  
}

void Delete(std::atomic<Node*>& aRoot, int aValue)
{
  Node* curNode = aRoot.load();
 
  for (;;)
  {
    if (curNode == nullptr)
    {
      return;
    }

    int curValue = curNode->iValue;
    if (curValue == aValue)
    {
      curNode->iDeleted.store(true);
      return;
    }
    if (aValue < curValue)
    {
      curNode = curNode->iLeft.load();
    }
    else if (aValue > curValue)
    {  
      curNode = curNode->iRight.load();
    }
  }  
}

void Print(std::atomic<Node*>& aRoot)
{
  Node* curNode = aRoot.load();
  if (curNode == nullptr) 
  {
    return;
  }

  Print(curNode->iLeft);
  if (!curNode->iDeleted.load())
  {
    std::cout << curNode->iValue << " ";
  }
  Print(curNode->iRight);
}

int main(int argc, char** argv)
{
  gAnchor.store(nullptr); 

  Add(gAnchor, 4);

  std::vector<std::thread> threads;

  for (int i = 0; i < 100; i++) 
  {
    threads.push_back(std::thread(Add, std::ref(gAnchor), i));
  }

  for (int i = 0; i < 100; i++) 
  {
    threads[i].join();
  }

  
  std::cout << Contains(gAnchor, 5) << std::endl;
  std::cout << Contains(gAnchor, 3) << std::endl;
   
  threads.clear();
  for (int i = 0; i < 100; i++) 
  {
    threads.push_back(std::thread(Delete, std::ref(gAnchor), i));
  }

  for (int i = 0; i < 100; i++) 
  {
    threads[i].join();
  }

  std::cout << Contains(gAnchor, 5) << std::endl;
  std::cout << Contains(gAnchor, 3) << std::endl;
  Print(gAnchor);
  std::cout << std::endl;
}
