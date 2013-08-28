function onSilverlightError(sender, args) {
    var appSource = "";
    if (sender != null && sender != 0) {
        appSource = sender.getHost().Source;
    }
    var errorType = args.ErrorType;
    var iErrorCode = args.ErrorCode;
    if (errorType == "ImageError" || errorType == "MediaError") {
        return;
    }
    var errMsg = "Unhandled Error in Silverlight Application " + appSource + "\n";
    errMsg += "Code: " + iErrorCode + "    \n";
    errMsg += "Category: " + errorType + "       \n";
    errMsg += "Message: " + args.ErrorMessage + "     \n";
    if (errorType == "ParserError") {
        errMsg += "File: " + args.xamlFile + "     \n";
        errMsg += "Line: " + args.lineNumber + "     \n";
        errMsg += "Position: " + args.charPosition + "     \n";
    }
    else if (errorType == "RuntimeError") {
        if (args.lineNumber != 0) {
            errMsg += "Line: " + args.lineNumber + "     \n";
            errMsg += "Position: " + args.charPosition + "     \n";
        }
        errMsg += "MethodName: " + args.methodName + "     \n";
    }
    throw new Error(errMsg);
}

function pluginLoaded(sender, args) {
    var slCtl = sender.getHost();

    window.WebSocketDraft = function (url) {
        this.slws = slCtl.Content.services.createObject("websocket");
        this.slws.Url = url;
        this.readyState = this.slws.ReadyState;
        var thisWs = this;
        this.slws.OnOpen = function (sender, args) {
            thisWs.readyState = thisWs.slws.ReadyState;
            if (thisWs.onopen) thisWs.onopen();
        };
        this.slws.OnData = function (sender, args) {
            if (thisWs.onmessage && args.TextData && args.IsFinal && !args.IsFragment)
                thisWs.onmessage({ data: String(args.TextData) });
        };
        this.slws.OnClose = function (sender, args) {
            thisWs.readyState = thisWs.slws.ReadyState;
            if (thisWs.onclose) thisWs.onclose();
        };
        this.slws.Open();
    };

    window.WebSocketDraft.prototype.send = function (message) {
        this.slws.SendMessage(message);
    };

    window.WebSocketDraft.prototype.close = function() {
        this.slws.Close();
    };

    $.slws._loaded = true;
    for (c in $.slws._callbacks) {
        $.slws._callbacks[c]();
    }
}

jQuery(function ($) {

    if (!$.slws) $.slws = {};
    else if (typeof ($.slws) != "object") {
        throw new Error("Cannot create jQuery.slws namespace: it already exists and is not an object.");
    }

    $(document).ready(function () {
        var script = document.createElement("script");
        document.body.appendChild(script);
        script.src = 'js/Silverlight.js';
        var slhost = document.createElement("div");
        document.body.insertBefore(slhost, document.body.firstChild);
        //document.body.appendChild(slhost);
        slhost.innerHTML =
        '<div align=center>' +
        '<object data="data:application/x-silverlight-2," type="application/x-silverlight-2" width="600" height="70">' +
		    '<param name="source" value="ClientBin/Microsoft.ServiceModel.Websockets.xap"/>' +
		    '<param name="onError" value="onSilverlightError" />' +
		    '<param name="background" value="white" />' +
		    '<param name="minRuntimeVersion" value="4.0.50401.0" />' +
		    '<param name="autoUpgrade" value="true" />' +
            '<param name="onLoad" value="pluginLoaded" />' +
		    '<a href="http://go.microsoft.com/fwlink/?LinkID=149156&v=4.0.50401.0" style="text-decoration:none">' +
 			    '<img src="http://go.microsoft.com/fwlink/?LinkId=161376" alt="Get Microsoft Silverlight" style="border-style:none"/>' +
		    '</a>' +
	    '</object><iframe id="_sl_historyFrame" style="visibility:hidden;height:0px;width:0px;border:0px"></iframe></div>';
    });

    $.slws._callbacks = [];

    $.slws.ready = function (callback) {
        if (callback) {
            if ($.slws._loaded) {
                callback();
            }
            else {
                $.slws._callbacks.push(callback);
            }
        }
    }
});