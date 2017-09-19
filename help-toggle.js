(function($) {
  Drupal.behaviors.farm_grazing_help = {
    attach: function(context, settings) {
      $("#block-system-help").hide();
      $(".region-help .glyphicon-question-sign").click(function() {
        $("#block-system-help").toggle();
      });
    }
  }
})(jQuery);
