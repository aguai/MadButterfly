function start_madbuilder() {
    var url = "chrome://MadBuilder/content/madbuilder.html";
    var browser = document.getElementById("content");

    browser.loadURI(url, null, null, true);
}
