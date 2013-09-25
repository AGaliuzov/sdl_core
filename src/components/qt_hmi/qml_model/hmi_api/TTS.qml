import QtQuick 2.0

Item {
<<<<<<< HEAD
=======
    property bool available: false

    signal onLanguageChange (int language)

>>>>>>> qtHMI_dev
    function isReady () {
        return {
            available: settings.ttsAvailable
        }
    }

    function getLanguage () {
        return {
            language: dataContainer.hmiTTSVRLanguage
        }
    }

    function getSupportedLanguages () {
        return {
            languages: settingsContainer.sdlLanguagesList
        }
    }

    function changeRegistration (language) {
        dataContainer.changeRegistrationTTSVR(language)
    }
}
