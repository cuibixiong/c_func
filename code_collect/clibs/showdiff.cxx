#include <iostream>
#include <string>

using namespace std;

string lcs(const string &s1, const string &s2);

void showDiff(const string &s1, const string &s2, const string );

int main(int argc, char **argv) {
    string s1, s2;
    cin >> s1 >> s2;
    string strRes = lcs(s1, s2);
    showDiff(s1, s2, strRes);
    return 0;
}

string lcs(const string &s1, const string &s2) {
    const int rowSize = s1.size() + 1;
    const int colSize = s2.size() + 1;
    string table[rowSize][colSize];
    char rowChar[rowSize];
    char colChar[colSize];
    int cnt = 0;
    rowChar[0] = colChar[0] = '/0';
    for (int i = rowSize - 2, cnt = 1; i >= 0; i--, cnt++) {
        rowChar[cnt] = s1[i];
    }
    for (int i = colSize - 2, cnt = 1; i >= 0; i--, cnt++) {
        colChar[cnt] = s2[i];
    }
    char ch1, ch2;
    string str1, str2;
    for (int i = 1; i < rowSize; i++) {
        for (int j = 1; j < colSize; j++) {
            ch1 = rowChar[i];
            ch2 = colChar[j];
            if (ch1 == ch2) {
                table[i][j] = ch1 + table[i - 1][j - 1];
            } else {
                str1 = table[i - 1][j];
                str2 = table[i][j - 1];
                if (str1.size() == str2.size()) {
                    table[i][j] = str1 < str2 ? str2 : str1;
                } else {
                    table[i][j] = str1.size() < str2.size() ? str2 : str1;
                }
            }
        }
    }

    return table[rowSize - 1][colSize - 1];
}

void showDiff(const string &s1, const string &s2, const string sub) {
    cout << "LSP: " + sub << endl;
    cout << endl;
    int tmp = s1.size();
    for (int i = 0, j = 0; i < tmp; i++) {
        if (s1[i] != sub[j]) {
            cout << '-';
        } else {
            cout << ' ';
            j++;
        }
    }
    cout << endl << s1 << endl;
    tmp = s2.size();
    for (int i = 0, j = 0; i < tmp; i++) {
        if (s2[i] != sub[j]) {
            cout << '+';
        } else {
            cout << ' ';
            j++;
        }
    }
    cout << endl << s2 << endl;
}
