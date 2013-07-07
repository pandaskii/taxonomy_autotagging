(function ($) {

    /**
     * Update the summary for the module's vertical tab.
     */
    Drupal.behaviors.taxonomy_autotaggingTAB = {
        attach: function (context) {
            // Use the fieldset class to identify the vertical tab element
            $('fieldset#edit-taxonomy-autotagging-tab', context).drupalSetSummary(function (context) {
                // Depending on the checkbox status, the settings will be customized, so
                // update the summary with the custom setting textfield string or a use a
                // default string.
                var selected = $('#edit-taxonomy-autotagging-tab-enabled input:checked', context);
                var count = 0;
                if (selected.size() > 0) {
                    count = selected.size();
                }
                if (count > 0) {
                    return Drupal.t('@count vocabulary enabled', {'@count': count});
                }
                else {
                    return Drupal.t('Not enabled');
                }
            });
        }
    };

})(jQuery);
