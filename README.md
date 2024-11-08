# BonDriverProxyEx
- [BonDriverProxyEx](#bondriverproxyex)
	- [BonDriverProxyExとは](#bondriverproxyexとは)
	- [BonDriverProxyとの設定方法の違い](#bondriverproxyとの設定方法の違い)
		- [1. サーバー側](#1-サーバー側)
		- [2. クライアント側](#2-クライアント側)
	- [サンプル](#サンプル)
	- [ビルド](#ビルド)
- [最後に](#最後に)
	- [LICENSE](#license)
	- [更新履歴](#更新履歴)

## BonDriverProxyExとは
オリジナルのテキストファイルをもとにMarkdown形式に変更しました。  
BonDriverProxyのサーバ側にBonDriver自動選択機能を加えたものです。  
dll側はバイナリ互換なので、BonDriverProxyの物を使用できます。  

<img src="image/image.png" width="50%">

## BonDriverProxyとの設定方法の違い
### 1. サーバー側
まずサーバ側のiniで、  
```ini
[BONDRIVER]  
00=PT-T;BonDriver_PT3-T0.dll;BonDriver_PT3-T1.dll  
01=PT-S;BonDriver_PT3-S0.dll;BonDriver_PT3-S1.dll  
```
の様な感じでチューナ空間とチャンネルの対応が同一のBonDriverを「;」区切りで並べます。  
なので、**グループ名やファイル名に「;」は使えません**  
<br>
- **BonDriverのグループ** は「00」「01」「02」...と言う感じで **「00」からの連番** で定義します。  
<br> 
- (標準では)「00」から「63」までの最大64種類定義できます。  

- 個々のグループのフォーマットは  
「グループ名;ドライバ1;ドライバ2;ドライバ3...」  
と言う感じになります。 

- **グループ名は必須** です。  
なお、1グループに指定できるドライバの数は、(これも標準では)最大63個まで  
- ＃あるいは、グループ名込みでの設定文字列の全体長がMAX_PATH(=260)*4文字まで  
となっています。  

### 2. クライアント側  
クライアント側のiniでは、  
```ini
[OPTION]  
BONDRIVER=PT-T  
```
の様な感じで、サーバ側iniのどのBonDriverグループを使用するか設定します。  
<br>
- 機能の要件上、一つのグループに二つ以上のドライバが設定されている場合、  
基本的には、クライアントからはそれらの内どれが使われるかは制御できなくなります。 

- 一応、グループ名の最後に **「:asc」及び「:desc」** を付けると、サーバ側で設定されている  
当該グループのドライバリストからの、**空きドライバの検索順序** を制御する事はできます。  

- **指定無し** の場合、あるいは **明示的に「:asc」** を指定した場合は、左から検索し、  
「:desc」を指定した場合は、右から検索します。  

- 例えば上記例の場合、  
	```ini
	BONDRIVER=PT-T:desc  
	```
	とした場合、BonDriver_PT3-T0.dllとBonDriver_PT3-T1.dllの両方が空きドライバであった場合、  
	BonDriver_PT3-T1.dllを使用するようになります。 
 
- ただし、あくまでも空きドライバの検索順序なので、空きが無い場合の使用ドライバをどれにするかを  
制御できるものではありません。  

- またこの機能の為、**グループ名として「:asc」もしくは「:desc」で終わる名称は使用できません ** 

<br>

- それら以外の設定は、元のBonDriverProxyと同じです。  
＃iniのファイル名の命名規則も同じで、サーバモジュールの実行ファイルやクライアントモジュールのdllの  
＃ファイル名の拡張子をiniに変更した物です。  
＃例えば、サーバモジュールが「BonDriverProxyEx.exe」なら「BonDriverProxyEx.ini」となります。  
<br>

- グループ内のどのBonDriverを選択するかのアルゴリズムは、  
	- 最初のリクエストではとりあえず可能であれば未使用の物を、それが無理ならなるべくチャンネルロックされていない  
  	物を優先して選択  
	- チャンネル変更時、同一グループの中で、既に目的のチャンネルを開いているのがいたらそれを共有  
  	その際、現在自分が持っているインスタンスは、他者と共有されていない限り開放する  
	- 共有状態からのチャンネル変更時は、最初のリクエストと同じ  
	- チャンネルロックされているインスタンスを共有している時のチャンネル変更は、未使用かチャンネルロックされていない  
  	インスタンスがある場合は使用インスタンスをそっちに切り替えて変更、無い場合は変更できない  

<br>

## サンプル
クライアント用iniファイル: [BonDriver_Proxy_Sample.ini](./BonDriver_Proxy_Sample.ini)  
サーバ用iniファイル: [BonDriverProxyEx_Sample](./BonDriverProxyEx_Sample.ini)  
クライアント用BonDriver_Proxyリポジトリ(fork版): [BonDriverProxy](https://github.com/stuayu/BonDriverProxy)

## ビルド
BonDriverProxyEx/aribb25 フォルダに[libaribb25](https://github.com/epgdatacapbon/libaribb25/tree/master/aribb25)をコピーの上  
BonDriverProxyEx.slnからビルドできます。  
（個人的にはRelease_DLLがお勧めです。）  
[MEMO.txt](./MEMO.txt)も参照ください。

# 最後に
こんな感じでしょうか？他にあるかな…  
ぶっちゃけ自分でも書いてて訳がわからなくなってますが、実際テストパターンの網羅性にイマイチ自信が持ててないので、  
何かしらバグがある可能性は高めです。  
よっしゃテストしたるわ、という方がいたら追試していただけたらと思います。  
<br>

## LICENSE
MITライセンスとします。
LICENSE.txt参照。

Jun/10/2014 unknown <unknown_@live.jp>

## 更新履歴
- version 1.1.6.6 (Aug/07/2016)
	- サーバ側にクライアントの接続状況を確認できるコマンドを追加した

- version 1.1.6.5 (Jan/17/2016)
	- 接続中のクライアントがいる場合はスリープを抑止するようにした

- version 1.1.6.4 (Nov/01/2015)
	- サーバ側の待ち受けIPアドレスを複数指定可能(ただし最大8つまで)にした

- version 1.1.6.3 (Sep/15/2015)
	- チャンネル変更を行う際に、その時点でのインスタンスがそのリクエストを出したクライアントに対して  
	  ロックされていなかった場合はそのままチャンネル変更していたのを、別インスタンスの最大優先度が  
	  現在のインスタンスの最大優先度よりも低かった場合にはそちらを選択するようにした  

- version 1.1.6.2 (Sep/13/2015)
	- サーバ側のTSパージ処理は実質不要なので簡素化した  
	  ＃TVTestはSetChannel()の前にPurgeTsStream()を呼ぶが、Ex版の場合はそのSetChannel()で別インスタンスを  
	  ＃使用するようになるかもしれないので、変更前インスタンスのTSデータをパージする必要は全く無い場合がある  
	  ＃この為、Ex版ではこの変更は特に都合が良い  

- version 1.1.6.1 (Sep/09/2015)
	- 排他ロックの先行優先に関して、2番手以降の排他権の獲得順序を厳密に要求順にした  

- version 1.1.6.0 (Sep/05/2015)
	- チャンネルロック機能で先行優先の排他ロックを可能にした  
	  ＃ついでに設定方式を数値による優先度指定方式にした  

- version 1.1.5.5 (May/07/2015)
	- サービス登録できるバージョンを追加した  
	- プロセスとスレッドの優先度を設定できるようにした  
	- UI有りバージョンにini再読み込み機能を追加した  
	- UI有りバージョンの情報窓を標準の閉じる操作で非表示にできるようにした  

- version 1.1.5.4 (May/06/2015)
	- タスクトレイから終了と接続クライアント情報の表示/非表示が出来るだけのUIを追加した  

- version 1.1.5.3 (Feb/23/2015)
	- BonDriverに対して実際にSetChannel()が行われた場合、それ以降に送信されるTSバッファとCNR値は、  
	  確実にチャンネル変更後に取得した物になるようにした  
	  ＃もっとも、結局はその先のBonDriver次第ではある  
	- TsReader()で使用する各変数を構造体にまとめた  

- version 1.1.5.2 (Feb/18/2015)
	- 特定条件において、ロック権を持っていないクライアントインスタンスの保持チャンネルと  
	  実際に選局されているチャンネルとの間にズレが生じる場合があったのを修正  

- version 1.1.5.1 (Feb/02/2015)
	- COMを使用するBonDriverへの対策を追加  
	  ＃BonDriverによっては内部でCOMの初期化/終了処理を行う物があるが、使い方によっては  
	  ＃初期化は呼ばれても終了処理が呼ばれない場合がある為、COMの初期化/終了処理を  
	  ＃サーバプロセス側で行っておくようにした  

- version 1.1.5.0 (Jan/30/2015)
	- サーバ側で一旦ロードしたBonDriverを、使用クライアントがいなくなった時にFreeLibrary()するのを  
	  許可しない設定を追加  

- version 1.1.4.11 (Jan/29/2015)
	- 通るべきCloseTuner()を通らないコードパスがあったのを修正  

- version 1.1.4.10 (Jan/28/2015)
	- BonDriverインスタンスを共有している複数のクライアントがほぼ同時にCloseTuner()した場合に、  
	  解放済みメモリにアクセスしてしまう場合があったのを修正  

- version 1.1.4.9 (Jan/28/2015)
	- Ex版のサーバに対してほぼ同時に複数のクライアントが接続してきた場合に、同一のBonDriverインスタンスに対して  
	  複数のTS配信スレッドをつくってしまう場合があったのを修正  

- version 1.1.4.8 (Jan/26/2015)
	- IBonDriver::Release()内部でのAccess Violation等の発生を無視する設定を追加

- version 1.1.4.7 (Dec/23/2014)
	- あるBonDriverグループに未使用BonDriverが無い状態で当該グループに更に使用要求が来た場合に、  
	  グループのBonDriverインスタンスにチャンネルロックされていない物が複数ある場合は、それらの中で  
	  ロード時刻あるいは使用要求時刻がもっとも古い物を、新規使用要求へのBonDriverインスタンスとして  
	  割り当てるようにした  

- version 1.1.4.6 (Dec/19/2014)
	- IPv6に対応した  
	- ホスト名として許可する長さを63文字以下から255文字以下にした  
	- サーバ側のlistenソケットのオプションをSO_EXCLUSIVEADDRUSEに変更した(winsockの仕様への対応)  
	- その他ロジックは変更無しでのソースコード整形  

- version 1.1.4.5 (Nov/21/2014)
	- あるBonDriverインスタンスをCloseTuner()する際はTS読み出しスレッドを必ず停止させておくようにした  
	- あるBonDriverインスタンスがCloseTuner()時に、チューナのオープン状態フラグが正しく反映されない  
	  パターンがあるのを修正  
	- レアケースだけどデッドロックするパターンへの対策を追加  
	- iniファイルが存在しなかった場合はエラーになるようにした  

- version 1.1.4.4 (Oct/17/2014)
	- OpenTuner()の呼び出しが完了した後、指定時間ディレイさせる事ができる機能を追加  
	  ＃BonDriver_PT(正確には、PT1/2-SDKを使用するBonDriver)へのad-hoc対応  
	  使用する場合は、サーバ側のiniの[OPTION]に追加で、  
	  ```ini
	  OPENTUNER_RETURN_DELAY=10
	  ```
	  の様な形で指定(単位はms)

- version 1.1.4.3 (Oct/11/2014)
	- 空きドライバの検索順序設定機能を追加

- version 1.1.4.2 (Aug/28/2014)
	- 受信スレッドの冗長なチェックを削除  
	- delete対象として、NULLでない場合が大半の物に付いてはNULLチェック削除  
	- delete後、即スコープを抜けるなどでNULLクリアが不要な物はNULLクリア削除  
	- その他ロジックは変更無しでのソースコード整形  

- version 1.1.4.0 (Jul/04/2014)
	- サーバ側のTSストリーム配信時のロック処理をロードしたBonDriverのインスタンス毎にグループ分けした  
	  これにより、グローバルなインスタンスリストのロックや他のBonDriverインスタンスのTSストリーム配信処理の影響を  
	  受けなくなるのと、個々のクライアントへのTSパケット作成時にロック及び比較処理が不要になる  
	- サーバ側のグローバルなインスタンスロック処理を厳密に行うのをデフォルトにした  
	  ＃詳細はBonDriverProxyの方のReadMe.txt参照  
	- チャンネル変更を行った際に、その対象BonDriverインスタンスを共有しているインスタンスが保持している  
	  スペース/チャンネル情報を更新していなかったのを修正  

- version 1.1.2.0 (Jun/19/2014)
	- SetChannel()内で、GetTsStream()でアプリに返したバッファを解放してしまうBonDriverを読み込んだ場合、  
	  タイミングが悪いと解放後メモリにアクセスしてしまう可能性があったのに対応  
	- その他ロジックは(ほぼ)変更無しでのソースコード整形  

- version 1.1.1.0 (Jun/10/2014)
	- 初版リリース
