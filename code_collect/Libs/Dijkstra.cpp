#include <iostream>
#include <fstream>
#include <vector>
#include <list>
using namespace std;
//邻接表中节点，每个节点与该节点对应的索引号指定一条边
struct Node
{
	int u; //边终点节点号
	int w; //边权值
	Node(int a, int b) :u(a), w(b){}
};
struct Record
{
	int pre; //路径中当前节点的前一节点
	int cost; //当前节点到源节点的最短路径代价
};
int n, m, s; //n表示图中节点个数，m表示图中边数，s表示源节点
vector<list<Node>> Adj; //图的邻接表
vector<Record> Path; //采用双亲表示法存储源节点到其他所有节点的最短路径信息
void Dijkstra()
{	
	vector<bool> isUsed(n, false); //向量某索引号对应的值为true，表示该索引号对应的节点
	//在S集合中
	list<int> Assi; //Assi中存储着当前的候选节点
	Path.assign(n, Record());
	//路径信息初始化
	for (int i = 0; i < n; i++)
	{
		Path[i].pre = i;
		Path[i].cost = INT_MAX;
	}
	isUsed[s] = true;
	for (auto it = Adj[s].begin(); it != Adj[s].end(); it++)
	{
		Path[it->u].pre = s;
		Path[it->u].cost = it->w;
		Assi.push_back(it->u);
	}
	while (!Assi.empty())
	{
		list<int>::iterator It;
		int minCost = INT_MAX;
		//从Assi中选择代价最小的节点加入到S集合中
		for (auto it = Assi.begin(); it != Assi.end(); it++)
		{
			if (minCost > Path[*it].cost)
			{
				minCost = Path[*it].cost;
				It = it;
			}
		}
		int u = *It;
		Assi.erase(It);
		isUsed[u] = true;
		//对与选中节点直接相连，并且不在S集合中的节点进行松弛操作
		//同时更新Assi的内容
		for (auto it = Adj[u].begin(); it != Adj[u].end(); it++)
		{
			if (isUsed[it->u]) continue;
			if (Path[it->u].cost == INT_MAX) Assi.push_back(it->u);
			if (Path[it->u].cost > minCost + it->w)
			{
				Path[it->u].cost = minCost + it->w;
				Path[it->u].pre = u;
			}
		}
	}
}
void Traverse(int k)
{
	if (Path[k].pre == k) { cout << k; return; }
	Traverse(Path[k].pre);
	cout << " " << k;
}
void Print()
{
	cout << "Result:\n";
	for (int i = 0; i < n; i++)
	{
		if (i == s) continue;
		cout << "From " << s << " to " << i << ": ";
		if (Path[i].cost == INT_MAX){ cout << "No path\n\n"; continue; }
		Traverse(i);
		cout << endl;
		cout << "Minimal Cost: " << Path[i].cost << endl << endl;
	}
}
int main()
{
	ifstream in("data.txt"); //从文件中读取图的信息
	in >> n >> m >> s;
	int u, v, w;
	Adj.assign(n, list<Node>());
	while (m--)
	{
		in >> u >> v >> w;
		Adj[u].push_back(Node(v, w));
	}
	in.close();

	Dijkstra();
	Print();

	system("pause");
	return 0;
}
