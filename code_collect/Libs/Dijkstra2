void Dijkstra(  
              const int numOfVertex,    /*节点数目*/  
              const int startVertex,    /*源节点*/  
              int (map)[][5],          /*有向图邻接矩阵*/  
              int *distance,            /*各个节点到达源节点的距离*/  
              int *prevVertex           /*各个节点的前一个节点*/  
              )  
{  
    vector<bool> isInS;                 //是否已经在S集合中  
    isInS.reserve(0);  
    isInS.assign(numOfVertex, false);   //初始化，所有的节点都不在S集合中  
      
    /*初始化distance和prevVertex数组*/  
    for(int i =0; i < numOfVertex; ++i)  
    {  
        distance[ i ] = map[ startVertex ][ i ];  
        if(map[ startVertex ][ i ] < INT_MAX)  
            prevVertex[ i ] = startVertex;  
        else  
            prevVertex[ i ] = -1;       //表示还不知道前一个节点是什么  
    }  
    prevVertex[ startVertex ] = -1;  
      
    /*开始使用贪心思想循环处理不在S集合中的每一个节点*/  
    isInS[startVertex] = true;          //开始节点放入S集合中  
      
      
    int u = startVertex;  
      
    for (int i = 1; i < numOfVertex; i ++)      //这里循环从1开始是因为开始节点已经存放在S中了，还有numOfVertex-1个节点要处理  
    {  
          
        /*选择distance最小的一个节点*/  
        int nextVertex = u;  
        int tempDistance = INT_MAX;  
        for(int j = 0; j < numOfVertex; ++j)  
        {  
            if((isInS[j] == false) && (distance[j] < tempDistance))//寻找不在S集合中的distance最小的节点  
            {  
                nextVertex = j;  
                tempDistance = distance[j];  
            }  
        }  
        isInS[nextVertex] = true;//放入S集合中  
        u = nextVertex;//下一次寻找的开始节点  
          
          
        /*更新distance*/  
        for (int j =0; j < numOfVertex; j ++)  
        {  
            if (isInS[j] == false && map[u][j] < INT_MAX)  
            {  
                int temp = distance[ u ] + map[ u ][ j ];  
                if (temp < distance[ j ])  
                {  
                    distance[ j ] = temp;  
                    prevVertex[ j ] = u;  
                }  
            }  
        }  
    }  
      
      
      
}  
