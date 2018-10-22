import QtQuick 2.8
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Axq 1.0

Window {
    visible: true
    width: 800
    height: 600
    title: qsTr("Hello World")

    Connections{
        target: Axq
        onError: console.log(errorString)
    }

    ColumnLayout{
        anchors.fill: parent

        Text{
            id: lines
        }

        RowLayout{
            Button{
                id: btn
                text: "pressME"
                enabled: false
                Component.onCompleted:{
                    var s = Axq.queue()
                        .meta(Axq.Index, function(value, index){
                            return index
                        })
                        .each(function(x){
                                lines.text = "" + x
                             })
                    clicked.connect(function(){
                        s.push()
                    })
                }
            }
            Button{
                text: "ASCII"
                id: ascii
                enabled: false
                onClicked: showAscii()
            }

            Button{
                text: "Split"
                id: split
                enabled: false
                onClicked: showSplit()
            }

            Button{
                text: "Scan"
                id: scan
                enabled: false
                onClicked: showScan()
            }

            Button{
                text: "Ask"
                id: ask
                enabled: false
                onClicked: askItem()
            }
        }
    }

    Component.onCompleted: splitString()

    function splitString(){
        Axq.from("What ever reads here, we handle".split(''))
        .onCompleted(function() {
            btn.enabled = true;
            ascii.enabled = true;
            split.enabled = true;
            scan.enabled = true;
            ask.enabled = true;
        })
        .each(function(x){lines.text += x})
        .request(200)
    }

    function showAscii() {
       lines.text = ""
       Axq.range(0, 255, 1).filter(
                    function(x){
                        return x > 0x20 && x < 0x7E
                    })
        .map(
             function(x){
                 return  x.toString() + ' ' + String.fromCharCode(x) + (x % 5 ? "\t" : '\n')
            })
        .each(
             function(acc){
                 lines.text += acc
             })
    }

    function showSplit() {
        lines.text = ""
        var array = new Array(20);
        for(var i = 0; i < array.length; i++)
            array[i] = i;
        Axq.iterator(array)
                .split('map', function(v){
                    return v * 10;
                })
            .each(function (lst) {
                lines.text += lst[0] + "->" + lst[1] + ";"
            })
    }


    function showScan() {
        lines.text = ""
        var nums = {one:'yksi', two:'kaksi', three:'kolme', four:'neljä', five:'viisi', six:'kuusi', seven:'seitsemän'}
        Axq.iterator(nums).scan("*", function(acc, v){lines.text += acc + "\n"; return acc + v;});
    }

    Rectangle {
        x: 100
        y: 100
        id: query
        visible: false
        width: 400
        height: 200
        color: "DarkGray"
        Rectangle {
            anchors.fill: parent
            anchors.margins: 30
            Column{
                anchors.fill: parent
                clip: true
                TextEdit {
                    id: query1
                    onVisibleChanged:  text = "Enter something, \"quit\" to finish"
                    onFocusChanged: text = ""
                }
                Text {
                    id: query2
                     onVisibleChanged: text = ""
                }
            }
        }
    }

    function askItem() {
        lines.text = ""
        query.visible = true
        var queue = Axq.queue()
        query1.onTextChanged.connect(function(){queue.push(query1.text)});
        queue.wait(function (txt) {
            query2.text = ""
            try{
                var it = Axq.iterator(txt)
                .map(function(c){
                        return c + "\t->" + c.charCodeAt(0) + "\t(" + c.charCodeAt(0).toString(16) + ")\n";
                    })
                .each(function (c){
                        query2.text += c
                    })
            } catch(e) {console.log(e)}
            return it
        })
        .complete(function(txt) {
            return txt === "quit"
        })
        .onComplete(function(){
            query.visible = false
        })
    }
}
