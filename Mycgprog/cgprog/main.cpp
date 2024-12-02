//g++ -O3 main.cpp -std=c++11 -framework OpenGL -framework GLUT -Wno-deprecated
//g++ main.cpp -framework OpenGL -framework GLUT -I/opt/homebrew/include -L/opt/homebrew/lib -lalut -framework OpenAL -Wno-deprecated
#include <iostream>
#include <GLUT/glut.h>  //OpenGL
#include <math.h>  //数学関数
#include <unistd.h>  //シリアル通信用
#include <fcntl.h>  //シリアル通信用
#include <termios.h>  //シリアル通信用
#include <AL/alut.h>//OpenAL
#include <stdio.h>
#include <stdlib.h>


//シリアルポート用
#define DEV_NAME "/dev/cu.M5stick_No3"  //M5StickC Plus2接続シリアルポート
#define BAUD_RATE B115200  //通信速度
#define BUFF_SIZE 4096
#define TILE 50  //格子頂点数

double T=0;
//三次元ベクトル構造体: Vec_3D
typedef struct _Vec_3D
{
    double x, y, z;
} Vec_3D;

// 建物の数
const int NUM_BUILDINGS = 200;

// 建物の位置、サイズ、色を格納
struct Building {
    float x, z;   // 位置
    float size;   // サイズ
    float r, g, b; // 色
};

// 建物のデータ
Building buildings[NUM_BUILDINGS];
//関数名の宣言
void initGL();
void initAL();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timer(int value);
void initSerial();
int getSerialData();
void analyzeBuffer();
void drawRoad(float x, float y, float z, float width, float length);
//void drawSolidBuilding(float x, float y, float z, float size, float r, float g, float b);
Vec_3D normcrossprod(Vec_3D v1,Vec_3D v2);//ベクトルが移籍計算関数
Vec_3D movevecnormalize(Vec_3D v1,Vec_3D v2);
double vectorNormalize(Vec_3D*vec);//ベクトル正規化用関数
void generateBuildings();
void drawSolidBuilding(float x, float y, float z, float size, float r, float g, float b);

//グローバル変数
double eDist, eDegX, eDegY;
double mX, mY, mState, mButton;
int winW, winH;
Vec_3D fPoint[TILE][TILE];

double fr = 60.0;  //フレームレート
double floorW= 1076.0,floorH = 691.0;//床の投影面サイズ
double deg = 0.0;//物体の回転角度
ALuint soundSource;//音源

//飛行機の関係
double x_move=0;
double y_speed=0.01;
double y_move=0;
double z_speed=0.1;
double z_move=0;
double speed = 0.01;
Vec_3D move_vec;//動く
Vec_3D player_vec={0,3,10};
//シリアル通信関係
int fd;  //シリアルポート
char bufferAll[BUFF_SIZE];  //蓄積バッファデータ
int bufferPoint = 0;  //蓄積バッファデータサイズ
double val[6];  //受信データ

//メイン関数
int main(int argc, char *argv[])
{
    initSerial();  //シリアルポート初期化
    glutInit(&argc, argv);  //OpenGL/GLUTの初期化
    alutInit(&argc, argv);  //OpenGL/GLUTの初期化
    generateBuildings(); // 建物をランダム生成
    initGL();  //初期設定
    initAL();  //OpenAL初期設定
    glutMainLoop();  //イベント待ち無限ループ
    
    return 0;
}

//ディスプレイコールバック関数
void display()
{
    val[1]=(val[1]*100);
    val[1]=(int)val[1]/10;
    double y_angle=-val[1]*5;
    if(y_move<y_angle){
        y_move+=0.1;
    }else{
        y_move-=0.1;
    }
    //y_move=70.0*-val[1];
    
    //左右方向の移動
    x_move+=0.5*val[0];
    //視点座標の計算
    Vec_3D e;
    double eDegX_move=eDegX+y_move;
    double eDegY_move=eDegY+x_move;
    e.x = eDist*cos(eDegX_move*M_PI/180.0)*sin(eDegY_move*M_PI/180.0)+player_vec.x;
    e.y = eDist*sin(eDegX_move*M_PI/180.0)+player_vec.y;
    e.z = eDist*cos(eDegX_move*M_PI/180.0)*cos(eDegY_move*M_PI/180.0)+player_vec.z;
    
    z_speed=0.01;
    z_move+=z_speed;
    //モデルビュー変換の設定
    
    glMatrixMode(GL_MODELVIEW);  //変換行列の指定（設定対象はモデルビュー変換行列）
    glLoadIdentity();  //行列初期化
    //y_move+=y_speed*val[1];
    // y_move+=y_speed;
    // y_move=0;
    //gluLookAt(0,0,0, 0.0, y_move-10, 0.0, 0.0, 1, 0.0);  //視点視線設定（視野変換行列を乗算）
    //gluLookAt(0,y_move, -z_move, e.x, e.y+y_move, e.z-z_move, 0.0, 1, 0.0);  //視点視線設定（視野変換行列を乗算）
    //gluLookAt(0,y_move, 0, e.x, e.y+y_move, e.z, 0.0, 1, 0.0);  //視点視線設定（視野変換行列を乗算）
    
    Vec_3D camera_vec={0,y_move, 10};
    Vec_3D point_vec={e.x,y_move+e.y,e.z+10};
    // move_vec = camera_vec-point_vec;
    // move_vec=vectorNormalize(move_vec);

    move_vec=movevecnormalize(player_vec,e);
    
    move_vec.x = move_vec.x*speed;
    move_vec.y = move_vec.y*speed;
    move_vec.z = move_vec.z*speed;
    player_vec.x -= move_vec.x ;
    player_vec.y -= move_vec.y ;
    player_vec.z -= move_vec.z ;
    // gluLookAt(0,y_move, 10, e.x, y_move+e.y, e.z+10, 0, 1, 0);  //視点視線設定（視野変換行列を乗算）
    gluLookAt(player_vec.x,player_vec.y, player_vec.z, e.x,e.y,e.z, -val[0]*1, 1, 0);  //視点視線設定（視野変換行列を乗算）
    //  glPushMatrix();
    // glTranslated(e.x,e.y,e.y);
    // glutSolidCube(1);
    // glPopMatrix();

    //光源０の位置指定
    GLfloat lightPos0[] = {0,5.0,5.0,1.0};//光源0の座標(x,y,z,距離の倍率 0だと無限の距離の光,1だと電球のような光)
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos0);//光源0の座標の設定
    static double theta = 0.0;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  //画面消去
    //光源0の位置指定
    GLfloat col[4],spe[4],shi[1];

    

    //全てを回転させる
    glPushMatrix();
    glRotated(0,0,0,1);
    glScaled(0.2,0.2,0.2);

    // 道路の描画
    drawRoad(0.0f, 0.0f, 0.0f, 10.0f, 40.0f);
    drawRoad(-10.0f, 0.0f, 10.0f, 40.0f, 10.0f);
    // 建物群の描画
    for (int i = 0; i < NUM_BUILDINGS; ++i) {
        drawSolidBuilding(
            buildings[i].x, 0.0f, buildings[i].z, 
            buildings[i].size, 
            buildings[i].r, buildings[i].g, buildings[i].b
        );
    }
    //床
    col[0] = .0;col[1]= 1.0;col[2] = 0.0;//拡散反射係数&環境後継すう(RGBA)
    spe[0] = 1.0;spe[1]= 1.0;spe[2] = 1.0;//鏡面反射係数(RGBA)
    shi[0] = 100.0;//ハイライト係数(p
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,col);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spe);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shi);
    glPushMatrix();
    glBegin(GL_QUADS);  //物体（四角形）頂点配置開始
    glNormal3b(0.0,1.0,0.0);//方線ベクトル設定;
    for (int j=0; j<TILE-1; j++) {
        for (int i=0; i<TILE-1; i++) {
            glVertex3d(fPoint[i][j].x, fPoint[i][j].y, fPoint[i][j].z);  //頂点
            glVertex3d(fPoint[i][j+1].x, fPoint[i][j+1].y, fPoint[i][j+1].z);  //頂点
            glVertex3d(fPoint[i+1][j+1].x, fPoint[i+1][j+1].y, fPoint[i+1][j+1].z);  //頂点
            glVertex3d(fPoint[i+1][j].x, fPoint[i+1][j].y, fPoint[i+1][j].z);  //頂点
        }
    }
    glEnd();  //物体頂点配置終了
    glPopMatrix();  //行列復帰
//四角
// ビルの描画
    // drawSolidBuilding(-5.0f, 0.0f, 5.0f, 3.0f, 0.5f, 0.5f, 0.5f);
    // drawSolidBuilding(5.0f, 0.0f, 5.0f, 4.0f, 0.3f, 0.7f, 0.3f);
    // drawSolidBuilding(-5.0f, 0.0f, -5.0f, 2.0f, 0.7f, 0.3f, 0.3f);
    // drawSolidBuilding(5.0f, 0.0f, -5.0f, 5.0f, 0.2f, 0.2f, 0.8f);
//三角形
    Vec_3D p0,p1,p2,v1,v2,v3;//三角形頂点＆辺ベクトル用
    Vec_3D n;//法線ベクトル用
    //頂点座標
    p0.x = 0;p0.y = 1.0;p0.z = 0;
    p1.x = -2.0;p1.y = -1.0;p1.z = 2.0;
    p2.x = 2.0;p2.y = -1.0;p2.z = 2.0;
    //辺ベクトル
    v1.x = p1.x-p0.x; v1.y = p1.y-p0.y; v1.z = p1.z-p0.z;
    v2.x = p2.x-p0.x; v2.y = p2.y-p0.y; v1.z = p2.z-p0.z;
    //方線ベクトル計算
    n = normcrossprod(v1,v2);//三角形p0-p1-p2の法線ベクトルは辺ベクトルv1とv2の外戚
    //材質
    col[0] = 0.9;col[1]= .0;col[2] = 0.9; col[3] = 1.0;//拡散反射係数&環境後継すう(RGBA)
    spe[0] = 1.0;spe[1]= 1.00;spe[2] = 1.0; col[3] = 1.0;//鏡面反射係数(RGBA)
    shi[0] = 100.0;//ハイライト係数(p
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,col);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spe);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shi);
    //配置
    glPushMatrix();//行列一時保存
    glNormal3d(n.x,n.y,n.z);//法線ベクトルの設定
    glBegin(GL_TRIANGLES);
    glVertex3d(p0.x,p0.y,p0.z);
    glVertex3d(p1.x,p1.y,p1.z);
    glVertex3d(p2.x,p2.y,p2.z);
    glEnd();
    glPopMatrix();
    
//三角形2
    //頂点座標
    p0.x = 0;p0.y = 1.0;p0.z = 0;
    p1.x = -2.0;p1.y = -1.0;p1.z = 2.0;
    p2.x = -2.0;p2.y = -1.0;p2.z = -2.0;
    //辺ベクトル
    v1.x = p1.x-p0.x; v1.y = p1.y-p0.y; v1.z = p1.z-p0.z;
    v2.x = p2.x-p0.x; v2.y = p2.y-p0.y; v1.z = p2.z-p0.z;
    //方線ベクトル計算
    n = normcrossprod(v1,v2);//三角形p0-p1-p2の法線ベクトルは辺ベクトルv1とv2の外戚
    //材質
    col[0] = 0.9;col[1]= .0;col[2] = 0.9; col[3] = 1.0;//拡散反射係数&環境後継すう(RGBA)
    spe[0] = 1.0;spe[1]= 1.00;spe[2] = 1.0; col[3] = 1.0;//鏡面反射係数(RGBA)
    shi[0] = 100.0;//ハイライト係数(p
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,col);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spe);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shi);
    //配置
    glPushMatrix();//行列一時保存
    glNormal3d(n.x,n.y,n.z);//法線ベクトルの設定
    glBegin(GL_TRIANGLES);
    glVertex3d(p0.x,p0.y,p0.z);
    glVertex3d(p1.x,p1.y,p1.z);
    glVertex3d(p2.x,p2.y,p2.z);
    glEnd();
    
    glPopMatrix();

//三角形3
    //頂点座標
    p0.x = 0;p0.y = 1.0;p0.z = 0;
    p1.x = -2.0;p1.y = -1.0;p1.z = -2.0;
    p2.x = 2.0;p2.y = -1.0;p2.z = -2.0;
    //辺ベクトル
    v1.x = p1.x-p0.x; v1.y = p1.y-p0.y; v1.z = p1.z-p0.z;
    v2.x = p2.x-p0.x; v2.y = p2.y-p0.y; v1.z = p2.z-p0.z;
    //方線ベクトル計算
    n = normcrossprod(v1,v2);//三角形p0-p1-p2の法線ベクトルは辺ベクトルv1とv2の外戚
    //材質
    col[0] = 0.9;col[1]= .0;col[2] = 0.9; col[3] = 1.0;//拡散反射係数&環境後継すう(RGBA)
    spe[0] = 1.0;spe[1]= 1.00;spe[2] = 1.0; col[3] = 1.0;//鏡面反射係数(RGBA)
    shi[0] = 100.0;//ハイライト係数(p
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,col);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spe);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shi);
    //配置
    glPushMatrix();//行列一時保存
    glNormal3d(n.x,n.y,n.z);//法線ベクトルの設定
    glBegin(GL_TRIANGLES);
    glVertex3d(p0.x,p0.y,p0.z);
    glVertex3d(p1.x,p1.y,p1.z);
    glVertex3d(p2.x,p2.y,p2.z);
    glEnd();
    glPopMatrix();

    //三角形3
    //頂点座標
    p0.x = 0;p0.y = 1.0;p0.z = 0;
    p1.x = 2.0;p1.y = -1.0;p1.z = -2.0;
    p2.x = 2.0;p2.y = -1.0;p2.z = 2.0;
    //辺ベクトル
    v1.x = p1.x-p0.x; v1.y = p1.y-p0.y; v1.z = p1.z-p0.z;
    v2.x = p2.x-p0.x; v2.y = p2.y-p0.y; v1.z = p2.z-p0.z;
    //方線ベクトル計算
    n = normcrossprod(v1,v2);//三角形p0-p1-p2の法線ベクトルは辺ベクトルv1とv2の外戚
    //材質
    col[0] = 0.9;col[1]= .0;col[2] = 0.9; col[3] = 1.0;//拡散反射係数&環境後継すう(RGBA)
    spe[0] = 1.0;spe[1]= 1.00;spe[2] = 1.0; col[3] = 1.0;//鏡面反射係数(RGBA)
    shi[0] = 100.0;//ハイライト係数(p
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,col);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spe);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shi);
    //配置
    glPushMatrix();//行列一時保存
    glNormal3d(n.x,n.y,n.z);//法線ベクトルの設定
    glBegin(GL_TRIANGLES);
    glVertex3d(p0.x,p0.y,p0.z);
    glVertex3d(p1.x,p1.y,p1.z);
    glVertex3d(p2.x,p2.y,p2.z);
    glEnd();
    glPopMatrix();
    glPopMatrix();
    glutSwapBuffers();  //描画実行
}

//タイマーコールバック関数
void timer(int value)
{
    glutTimerFunc(1000/fr, timer, 0);  //タイマー再設定・・・timer()関数が1000/fr秒ごとに呼び出される
    glutPostRedisplay();  //ディスプレイイベント強制発生・・・display()関数が呼び出される
    
    getSerialData();  //加速度：val[0],val[1],val[2]，角速度val[3], val[4], val[5]を入手
    printf("(%f, %f, %f) (%f, %f, %f)\n", val[0], val[1], val[2], val[3], val[4], val[5]);
    if(fabs(val[0])>3){
        
        alSourcePlay(soundSource);
    }
    
    alSourcef(soundSource,AL_PITCH,fabs(val[0]+1.0));//ピッチ設定
    deg +=val[5]/fr;//物体の回転角度の更新　フレームレートで割る
}

//シリアルポートからデータ取得
int getSerialData()
{
    //シリアルポートからの入力用
    char buffer[BUFF_SIZE];
    //buffer[]に受信データ格納
    int len = read(fd, buffer, BUFF_SIZE);
    
    //bufferAll[]に蓄積
    for (int i=0; i<len; i++) {
        bufferAll[bufferPoint] = buffer[i];  //bufferAll[]にbuffer[]を蓄積
        bufferPoint++;  //バッファサイズ追加
        
        if (buffer[i]=='\n') {  //読み込んだ文字が'\n'(改行)なら受信データの最後
            analyzeBuffer();  //データの解析
            bufferPoint = 0;  //bufferAll[]をリセット
        }
    }
    
    return len;
}

//bufferAll[]分析
void analyzeBuffer()
{
    char inData[6][BUFF_SIZE];
    int dataID = 0;  //データ番号
    int j = 0;  //文字位置
    
    for (int i=0; i<bufferPoint; i++) {
        inData[dataID][j] = bufferAll[i];  //bufferALl[]の値をinData[][j]に1文字ずつ格納
        
        if (inData[dataID][j]==',') {  //カンマ（数値データの区切り）のとき
            inData[dataID][j] = '\0';  //文字列inData[dataID]完成
            dataID++;  //次の数値データ（文字列）に移動
            if (dataID==6) break;  //6を超えないように
            j = 0;  //次の文字に移動
        }
        else if (inData[dataID][j]=='\n') {  //改行コード（受信データセットの最後）のとき
            inData[dataID][j] = '\0';  //文字列生成
            dataID++;  //次の数値データに移動
            break;  //全データの取得完了
        }
        else {  //データの途中
            j++;  //次の文字に移動
        }
    }
    
    if (dataID==6) {  //データが完全に揃っているとき
        //inData[i][]（i=0~5）の文字列を数値化してval[i]に格納
        for (int i=0; i<6; i++) {
            //文字列を小数に変換
            val[i] = atof(inData[i]);
        }
    }
}

//初期化関数
void initGL()
{
   //描画ウィンドウ生成
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);  //ディスプレイモードの指定
    glutInitWindowSize(floorW, floorH);  //ウィンドウサイズの指定
    glutCreateWindow("CG");  //ウィンドウの生成
    
    //コールバック関数の指定
    glutDisplayFunc(display);  //ディスプレイコールバック関数の指定（"display()"）
    glutReshapeFunc(reshape);  //リサイズコールバック関数の指定（"reshape()"）
    glutKeyboardFunc(keyboard);  //キーボードコールバック関数の指定（"keyboard()"）
    glutMouseFunc(mouse);  //マウスクリックコールバック関数の指定（"mouse()"）
    glutMotionFunc(motion);  //マウスドラッグコールバック関数の指定（"motion()"）
    glutTimerFunc(1000/fr, timer, 0);  //タイマーコールバック関数（"timer"）
    
    //各種設定
    glClearColor(0.0, 0.0, 0.2, 1.0);  //ウィンドウクリア色の指定（RGBA）
    glEnable(GL_DEPTH_TEST);  //デプスバッファの有効化
    glEnable(GL_NORMALIZE);
    //光源設定
    GLfloat col[4];//RBGA値設定
    glEnable(GL_LIGHTING);//ライティング始め
    glEnable(GL_LIGHT0);//光源０有効か　LIGHT0はrgbが全て1
    col[0]=1.0;col[1]=1.0,col[2]=1.0;col[3]= 1.0;//RBGA
    glLightfv(GL_LIGHT0,GL_DIFFUSE,col);//光源0の拡散反射に関する強度
    glLightfv(GL_LIGHT0,GL_SPECULAR,col);//光源0の拡散反射に関する強度
    col[0]=0.2;col[1]=0.2,col[2]=0.2;col[3]= 0.2;//RBGA
    glLightfv(GL_LIGHT0,GL_AMBIENT,col);//光源0の拡散反射に関する強度
   
    //GLfloat col[4],spe[4],shi[1];
    //視点関係
    eDist = 15.0; eDegX = 180.0; eDegY = 0.0;
    
    //床面頂点
    for (int j=0; j<TILE; j++) {
        for (int i=0; i<TILE; i++) {
            fPoint[i][j].x = -4.0+i*8.0/(TILE-1);
            fPoint[i][j].z = -4.0+j*8.0/(TILE-1);
            fPoint[i][j].y = -1.0;
        }
    }
}
//OpenAL初期設定
void initAL()
{

    ALuint soundBuffer; //バッファ
    alGenBuffers(1,&soundBuffer);//バッファ生成

    //音源生成
    soundBuffer = alutCreateBufferFromFile("pon.wav");//バッファにサウンドファイル取り込み
    alGenSources(1,&soundSource);//音源生成
    alSourcei(soundSource,AL_BUFFER,soundBuffer);//音源にバッファを結びつけ
    alSourcei(soundSource,AL_LOOPING,AL_TRUE);//ループ再生設定(オフ)
    alSourcei(soundSource,AL_PITCH,1.0);//ピッチ設定
    alSourcei(soundSource,AL_GAIN,1.0);//音量設定（1が最大)
}
//リシェイプコールバック関数
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);  //ウィンドウ内の描画領域(ビューポート)の指定
    //投影変換の設定
    glMatrixMode(GL_PROJECTION);  //変換行列の指定（設定対象は投影変換行列）
    glLoadIdentity();  //行列初期化
    gluPerspective(30.0, (double)w/(double)h, 1.0, 1000.0);  //透視投影ビューボリューム設定
    winW = w; winH = h;  //ウィンドウサイズ保存
}

//キーボードコールバック関数(key:キーの種類，x,y:座標)
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 27:
            exit(0);  //終了
        case 'w':
            y_speed=0.01;
            y_move-=10;
            //alSourcePlay(soundSource);
            break;
        case 's':
            y_speed=-0.01;
            y_move+=10;
            //alSourcePlay(soundSource);
            break;
        case 'a':
            y_speed=0.01;
            // y_move+=10;
            x_move+=10;
            //alSourcePlay(soundSource);
            break;
        case 'd':
            y_speed=-0.01;
            x_move-=10;
            //alSourcePlay(soundSource);
            break;
        default:
            break;
    }
}

//シリアルポート初期化
void initSerial()
{
    struct termios tio;
    
    //シリアルポートオープン
    fd = open(DEV_NAME, O_RDWR | O_NONBLOCK );
    if(fd<0) { //オープン失敗
        printf("ERROR on device open\n");
        exit(1);
    }
    printf("init serial port\n");
    
    //初期設定
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_cc[VTIME] = 100;
    //ボーレート設定
    cfsetispeed(&tio, BAUD_RATE);
    cfsetospeed(&tio, BAUD_RATE);
    //デバイス設定実施
    tcsetattr(fd, TCSANOW, &tio);
}


//マウスクリックコールバック関数
void mouse(int button, int state, int x, int y)
{
    //マウスボタンが押されたとき，マウス座標をグローバル変数に保存
    if (state==GLUT_DOWN) {
        mX = x; mY = y; mButton = button; mState = state;
    }
}

//マウスドラッグコールバック関数
void motion(int x, int y)
{
    //マウスの移動量を角度変化量に変換
    eDegY = eDegY+(mX-x)*0.5;  //マウス横方向→水平角
    eDegX = eDegX+(y-mY)*0.5;  //マウス縦方向→垂直角
    
    //マウス座標をグローバル変数に保存
    mX = x; mY = y;
}


//ベクトルの外積計算
Vec_3D normcrossprod(Vec_3D v1,Vec_3D v2)
{
    Vec_3D out;
    out.x = v1.y*v2.z - v1.z*v2.y;
    out.y = v1.z*v2.x - v1.x*v2.z;
    out.z = v1.x*v2.y - v1.y*v2.x;
    vectorNormalize(&out);
    //戻り値
    return out;//戻り値は外積ベクトル
}
//ベクトルの外積計算
Vec_3D movevecnormalize(Vec_3D v1,Vec_3D v2)
{
    Vec_3D out;
    out.x = v1.x - v2.x;
    out.y = v1.y - v2.y;
    out.z = v1.z - v2.z;
    vectorNormalize(&out);
    //戻り値
    return out;//戻り値は外積ベクトル
}
double vectorNormalize(Vec_3D*vec){
    double len;
    len = sqrt(vec->x*vec->x+vec->y*vec->y+vec->z*vec->z);
    if(len>0){
        vec->x = vec->x/len;
        vec->y = vec->y/len;
        vec->z = vec->z/len;
    }
    return len;
}

// void drawSolidBuilding(float x, float y, float z, float size, float r, float g, float b) {
//     glPushMatrix();
//     glTranslatef(x, y + size / 2.0f, z); // 中心を底面に合わせる
//     glColor3f(r, g, b); // 色を設定
//     glutSolidCube(size); // 立方体を描画
//     glPopMatrix();
// }

// 道路を描画する関数
void drawRoad(float x, float y, float z, float width, float length) {
    glPushMatrix();
    glTranslatef(x, y + 0.01f, z); // 少し上に上げて重なりを防ぐ
    glScalef(width, 0.02f, length); // 幅と長さを調整
    glColor3f(0.1f, 0.1f, 0.1f); // 道路の色
    glutSolidCube(1.0f); // 立方体をスケールして道路を描画
    glPopMatrix();
}

// 建物を描画
void drawSolidBuilding(float x, float y, float z, float size, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y + size / 2.0f, z); // 中心を底面に合わせる
    glColor3f(r, g, b); // 色を設定
    glutSolidCube(size); // 立方体を描画
    glPopMatrix();
}

// 建物をランダムに生成
void generateBuildings() {
    srand(static_cast<unsigned int>(time(0))); // ランダムシード
    for (int i = 0; i < NUM_BUILDINGS; ++i) {
        buildings[i].x = (rand() % 200) - 100;      // -100から100の範囲
        buildings[i].z = (rand() % 200) - 100;     // -100から100の範囲
        buildings[i].size = (rand() % 10) + 1;     // サイズは1〜10
        buildings[i].r = static_cast<float>(rand() % 100) / 100.0f; // 0.0〜1.0
        buildings[i].g = static_cast<float>(rand() % 100) / 100.0f; // 0.0〜1.0
        buildings[i].b = static_cast<float>(rand() % 100) / 100.0f; // 0.0〜1.0
    }
}